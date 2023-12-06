/*
Copyright (C) 2011 Georgia Institute of Technology

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

/*
* IIRfilter
* Computes IIR filter coefficients
*
*/

#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QCheckBox>
#include <algorithm>
#include <numeric>
#include <rtxi/dsp/log2.h>
#include "widget.hpp"

enum PARAMETER : Widgets::Variable::Id
{
	FILTER_ORDER=0,
	PASSBAND_RIPPLE,
	PASSBAND_EDGE,
	STOPBAND_RIPPLE,
	STOPBAND_EDGE,
	INPUT_QUANTIZING_FACTOR,
	COEFF_QUANTIZING_FACTOR,
	FILTER_TYPE,
	CHEBYSHEV_NORM_TYPE,
	PREDISTORT,
	QUANTIZE
};

inline std::vector<Widgets::Variable::Info> get_default_vars()
{
//set up parameters, calls for initialization, creation, update, and refresh of GUI
	return {
		{FILTER_ORDER,           "Filter Order", "Filter Order", Widgets::Variable::INT_PARAMETER, int64_t{10}},
		{PASSBAND_RIPPLE,        "Passband Ripple (dB)", "Passband Ripple (dB)", Widgets::Variable::DOUBLE_PARAMETER, 3.0},
		{PASSBAND_EDGE,          "Passband Edge (Hz)", "Passband Edge (Hz)", Widgets::Variable::DOUBLE_PARAMETER, 60.0},
		{STOPBAND_RIPPLE,        "Stopband Ripple (dB)", "Stopband Ripple (dB)", Widgets::Variable::DOUBLE_PARAMETER, 60.0},
		{STOPBAND_EDGE,          "Stopband Edge (Hz)", "Stopband Edge (Hz)", Widgets::Variable::DOUBLE_PARAMETER, 200.0},
		{INPUT_QUANTIZING_FACTOR,"Input quantizing factor", "Bits eg. 10, 12, 16", Widgets::Variable::INT_PARAMETER, int64_t{4096}},
		{COEFF_QUANTIZING_FACTOR,"Coefficients quantizing factor", "Bits eg. 10, 12, 16", Widgets::Variable::INT_PARAMETER, int64_t{4096}},
		{FILTER_TYPE,		 "Type of filter to implement", "Butterworth, Chebyshev, Elliptical", Widgets::Variable::UINT_PARAMETER, uint64_t{0}},
		{CHEBYSHEV_NORM_TYPE,	 "Chebyshev normalization type", "", Widgets::Variable::UINT_PARAMETER, uint64_t{0}},
		{PREDISTORT,	 "Pre-Distort Signal", "", Widgets::Variable::UINT_PARAMETER, uint64_t{1}},
		{QUANTIZE,	 "Use Quantization Mode", "", Widgets::Variable::UINT_PARAMETER, uint64_t{0}}
	};
}

inline std::vector<IO::channel_t> get_default_channels()
{
//set up inputs/outputs, calls for initialization, creation, update, and refresh of GUI
	return {
		{ "Input", "Input to Filter", IO::INPUT, },
		{ "Output", "Output of Filter", IO::OUTPUT }
	};
}

IIRfilter::IIRfilter(QMainWindow* main_window, Event::Manager* ev_manager) 
	: Widgets::Panel(std::string("IIR Filter"), main_window, ev_manager) 
{
	setWhatsThis(
	"<p><b>IIR Filter:</b><br>This plugin computes filter coefficients for three types of IIR filters. "
	"They require the following parameters: <br><br>"
	"Butterworth: passband edge <br>"
	"Chebyshev: passband ripple, passband edge, ripple bw_norm <br>"
	"Elliptical: passband ripple, stopband ripple, passband edge, stopband edge <br><br>"
	"Since this plug-in computes new filter coefficients whenever you change the parameters, you should not"
	"change any settings during real-time.</p>");
	
	Widgets::Panel::createGUI(get_default_vars(), {FILTER_TYPE, PREDISTORT, QUANTIZE, CHEBYSHEV_NORM_TYPE});
	customizeGUI();
	QTimer::singleShot(0, this, SLOT(resizeMe()));
}

IIRfilterComponent::IIRfilterComponent(Widgets::Plugin* host_plugin) 
	: Widgets::Component(host_plugin, "IIR Filter", get_default_channels(), get_default_vars())
{}
				
//execute, the code block that actually does the signal processing
void IIRfilterComponent::execute() {
	switch (this->getState()) {
		case RT::State::EXEC:
			writeoutput(0, filter_implem->ProcessSample(readinput(0)));
			break;
		case RT::State::INIT:
		case RT::State::MODIFY:
			filter_order = getValue<int64_t>(FILTER_ORDER);
			passband_ripple = getValue<double>(PASSBAND_RIPPLE);
			passband_edge = getValue<double>(PASSBAND_EDGE);
			stopband_ripple = getValue<double>(STOPBAND_RIPPLE);
			stopband_edge = getValue<double>(STOPBAND_EDGE);
			stopband_edge *= TWO_PI;
			filter_type = static_cast<filter_t>(getValue<uint64_t>(FILTER_TYPE));
			input_quan_factor = 2 ^ getValue<int64_t>(INPUT_QUANTIZING_FACTOR); // quantize input to 12 bits
			coeff_quan_factor = 2 ^ getValue<int64_t>(COEFF_QUANTIZING_FACTOR); // quantize filter coefficients to 12 bits
			ripple_bw_norm = getValue<uint64_t>(CHEBYSHEV_NORM_TYPE);
			predistort_enabled = getValue<uint64_t>(PREDISTORT) == 1;
			quant_enabled = getValue<uint64_t>(QUANTIZE) == 1;
			makeFilter();
			writeoutput(0, filter_implem->ProcessSample(readinput(0)));
			this->setState(RT::State::EXEC);
			break;
	
		case RT::State::PAUSE:
			writeoutput(0, 0); // stop command in case pause occurs in the middle of command
			break;
	
		case RT::State::UNPAUSE:
			this->setState(RT::State::EXEC);
			writeoutput(0,0);
			break;
	
		case RT::State::PERIOD:
			dt = RT::OS::getPeriod() * 1e-9; // s
			this->setState(RT::State::EXEC);
			break;
		default:
			break;
	}
}

std::vector<double> IIRfilterComponent::getNumeratorCoefficients()
{
	double* numerator_coeff = this->filter_design->GetNumerCoefficients();
	int numer_count = this->filter_design->GetNumNumerCoeffs();
	std::vector<double> result(numerator_coeff, numerator_coeff+numer_count);
	return result;
}

std::vector<double> IIRfilterComponent::getDenominatorCoefficients()
{
	double* denominator_coeff = this->filter_design->GetDenomCoefficients();
	int denomin_count = this->filter_design->GetNumDenomCoeffs();
	std::vector<double> result(denominator_coeff, denominator_coeff+denomin_count);
	return result;
}

// custom functions, as defined in the header file
void IIRfilterComponent::initParameters() {
	dt = RT::OS::getPeriod() * 1e-9; // s
	filter_type = BUTTER;
	filter_order = 10;
	passband_ripple = 3;
	passband_edge = 60;
	stopband_ripple = 60;
	stopband_edge = 200;
	ripple_bw_norm = 0;
	predistort_enabled = true;
	quant_enabled = false;
	input_quan_factor = 4096; // quantize input to 12 bits
	coeff_quan_factor = 4096; // quantize filter coefficients to 12 bits
	makeFilter();
}

void IIRfilter::updateFilterType(int index) {
	if(index < 0) { return; }
	int result = this->getHostPlugin()->setComponentParameter<uint64_t>(FILTER_TYPE, static_cast<uint64_t>(index));	
	if(result < 0){
		ERROR_MSG("IIRfilter::updateFilterType : Unable to change filter type"); 
	}
	this->update_state(RT::State::MODIFY);
}

void IIRfilter::updateNormType(int index) {
	if(index < 0) { return; }
	int result = this->getHostPlugin()->setComponentParameter<uint64_t>(CHEBYSHEV_NORM_TYPE, static_cast<uint64_t>(index));
	if(result < 0){
		ERROR_MSG("IIRfilter::updateFilterType : Unable to change filter normalization type"); 
	}
	this->update_state(RT::State::MODIFY);
}

void IIRfilterComponent::makeFilter() {
	delete analog_filter;
	switch (filter_type) {
		case BUTTER:
			analog_filter = new ButterworthTransFunc(filter_order);
			analog_filter->LowpassDenorm(passband_edge);
			break;

		case CHEBY:
			analog_filter = new ChebyshevTransFunc(filter_order, passband_ripple,
			ripple_bw_norm);
			analog_filter->LowpassDenorm(passband_edge);
			break;
	
		case ELLIP:
			int upper_summation_limit = 5;
			analog_filter = new EllipticalTransFunc(filter_order, passband_ripple,
			stopband_ripple, passband_edge, stopband_edge, upper_summation_limit);
			break;
	} // end of switch on window_shape
	
	if (predistort_enabled) analog_filter->FrequencyPrewarp(dt);
	
	delete filter_design;
	filter_design = BilinearTransf(analog_filter, dt);

	delete filter_implem;
	if (quant_enabled) {
		filter_implem = new DirectFormIir(filter_design->GetNumNumerCoeffs(),
			filter_design->GetNumDenomCoeffs(),
			filter_design->GetNumerCoefficients(),
			filter_design->GetDenomCoefficients(), coeff_quan_factor,
			input_quan_factor);
	} else {
		filter_implem = new UnquantDirectFormIir(
			filter_design->GetNumNumerCoeffs(),
			filter_design->GetNumDenomCoeffs(),
			filter_design->GetNumerCoefficients(),
			filter_design->GetDenomCoefficients());
	}
}

void IIRfilter::saveIIRData() {
	QFileDialog* fd = new QFileDialog(this, "Save File As"/*, TRUE*/);
	fd->setFileMode(QFileDialog::AnyFile);
	fd->setViewMode(QFileDialog::Detail);
	QString fileName;
	auto* host_plugin = dynamic_cast<IIRfilterPlugin*>(this->getHostPlugin());
	if (fd->exec() == QDialog::Accepted) {
		QStringList files = fd->selectedFiles();
		if (!files.isEmpty()) fileName = files.takeFirst();
		
		if (OpenFile(fileName)) {
			// stream.setPrintableData(true);
			switch (this->filterType->currentIndex()) {
				case 0:
					stream << QString("BUTTERWORTH order=") << host_plugin->getComponentUIntParameter(FILTER_ORDER)
						<< " passband edge=" << host_plugin->getComponentDoubleParameter(PASSBAND_EDGE);
					break;

				case 1:
					stream << QString("CHEBYSHEV order=") << host_plugin->getComponentUIntParameter(FILTER_ORDER)

						<< " passband ripple=" << host_plugin->getComponentDoubleParameter(PASSBAND_RIPPLE)
						<< " passband edge=" << host_plugin->getComponentDoubleParameter(PASSBAND_EDGE);

					if (host_plugin->getComponentUIntParameter(CHEBYSHEV_NORM_TYPE) == 0) stream << " with 3 dB bandwidth normalization";
					else stream << " with ripple bandwidth normalization";
					break;
		
				case 2:
					stream << QString("ELLIPTICAL order=") << host_plugin->getComponentUIntParameter(FILTER_ORDER)
						<< " passband ripple=" << host_plugin->getComponentDoubleParameter(PASSBAND_RIPPLE)
						<< " passband edge=" << host_plugin->getComponentDoubleParameter(PASSBAND_EDGE)
						<< " stopband ripple=" << host_plugin->getComponentDoubleParameter(STOPBAND_RIPPLE)
						<< " stopband edge=" << host_plugin->getComponentDoubleParameter(STOPBAND_EDGE);
					break;
			}
			stream << QString(" \n");
		
			std::vector<double> numer_coeff = host_plugin->getIIRfilterNumeratorCoefficients();
			std::vector<double> denom_coeff = host_plugin->getIIRfilterDenominatorCoefficients();
			
			stream << QString("Filter numerator coefficients:\n");
			for (int i = 0; i < numer_coeff.size(); i++) {
				stream << QString("numer_coeff[") << i << "] = "
					<< (double) numer_coeff[i] << "\n";
			}
			stream << QString("Filter denominator coefficients:\n");
			for (int i = 0; i < denom_coeff.size(); i++) {
				stream << QString("denom_coeff[") << i << "] = "
					<< (double) denom_coeff[i] << "\n";
			}
			dataFile.close();
		}
		else {
			QMessageBox::information(this, "IIR filter: Save filter parameters",
			"There was an error writing to this file. You can view\n"
			"the parameters in the terminal.\n");
		}
	}
}

bool IIRfilter::OpenFile(QString FName) {
	dataFile.setFileName(FName);
	if (dataFile.exists()) {
		switch (QMessageBox::warning(this, "IIR filter", tr(
				"This file already exists: %1.\n").arg(FName), "Overwrite", "Append",
				"Cancel", 0, 2)){
			case 0: // overwrite
				dataFile.remove();
				if (!dataFile.open(QIODevice::Unbuffered | QIODevice::WriteOnly)) return false;
				break;
	
			case 1: // append
				if (!dataFile.open(QIODevice::Unbuffered | QIODevice::WriteOnly | QIODevice::Append)) return false;
				break;
			
			case 2: // cancel
				return false;
				break;
		}
	} else if (!dataFile.open(QIODevice::Unbuffered | QIODevice::WriteOnly)) return false;

	stream.setDevice(&dataFile);
	//	stream.setPrintableData(false); // write binary
	return true;
}

//create the GUI components
void IIRfilter::customizeGUI(void) {
	//QGridLayout *customLayout = Widgets::Panel::getLayout();
	auto* customLayout = dynamic_cast<QVBoxLayout*>(this->layout());

	//QVBoxLayout *customGUILayout = new QVBoxLayout;
	
	QPushButton *saveDataButton = new QPushButton("Save IIR Coefficients");
	//customGUILayout->addWidget(saveDataButton);
	customLayout->addWidget(saveDataButton, 0);
	QObject::connect(saveDataButton, SIGNAL(clicked()), this, SLOT(saveIIRData()));
	saveDataButton->setToolTip("Save filter parameters and coefficients to a file");

	auto* topGroup = new QGroupBox("Filter Types");
	QFormLayout *optionLayout = new QFormLayout(topGroup);
	//customGUILayout->addLayout(optionLayout);
	
	filterType = new QComboBox;
	filterType->setToolTip("IIR filter.");
	filterType->insertItem(0, "Butterworth");
	filterType->insertItem(1, "Chebyshev");
	filterType->insertItem(2," Elliptical");
	optionLayout->addRow("IIR filter", filterType);
	QObject::connect(filterType,SIGNAL(activated(int)), this, SLOT(updateFilterType(int)));
	
	normType = new QComboBox;
	normType->insertItem(0, "3 dB bandwidth");
	normType->insertItem(1, "Ripple bandwidth");
	normType->setToolTip("Type of Chebyshev normalization");
	optionLayout->addRow("Chebyshev Normalize Type:", normType);
	QObject::connect(normType,SIGNAL(activated(int)), this, SLOT(updateNormType(int)));
	normType->setEnabled(false);
	customLayout->insertWidget(0, topGroup);

	auto* checkboxGroup = new QGroupBox("Finetunning");
	QFormLayout *checkBoxLayout = new QFormLayout(checkboxGroup);
	QCheckBox *predistortCheckBox = new QCheckBox;
	checkBoxLayout->addRow("Predistort frequencies", predistortCheckBox);
	QCheckBox *quantizeCheckBox = new QCheckBox;
	checkBoxLayout->addRow("Quantize input and coefficients", quantizeCheckBox);
	QObject::connect(predistortCheckBox,SIGNAL(toggled(bool)),this,SLOT(togglePredistort(bool)));
	QObject::connect(quantizeCheckBox,SIGNAL(toggled(bool)),this,SLOT(toggleQuantize(bool)));
	predistortCheckBox->setToolTip("Predistort frequencies for bilinear transform");
	quantizeCheckBox->setToolTip("Quantize input and coefficients");
	
	customLayout->insertWidget(1, checkboxGroup);
	setLayout(customLayout);	
}

void IIRfilter::togglePredistort(bool on) {
	uint64_t val = on ? 1 : 0;
	this->getHostPlugin()->setComponentParameter(PREDISTORT, val);
}

void IIRfilter::toggleQuantize(bool on) {
	uint64_t val = on ? 1 : 0;
	this->getHostPlugin()->setComponentParameter(QUANTIZE, val);
}

IIRfilterPlugin::IIRfilterPlugin(Event::Manager* ev_manager) : Widgets::Plugin(ev_manager, "IIR Filter") {}

std::vector<double> IIRfilterPlugin::getIIRfilterNumeratorCoefficients()
{
	return dynamic_cast<IIRfilterComponent*>(this->getComponent())->getNumeratorCoefficients();
}

std::vector<double> IIRfilterPlugin::getIIRfilterDenominatorCoefficients()
{
	return dynamic_cast<IIRfilterComponent*>(this->getComponent())->getDenominatorCoefficients();
}

//create plug-in
std::unique_ptr<Widgets::Plugin> createRTXIPlugin(Event::Manager* ev_manager)
{
  return std::make_unique<IIRfilterPlugin>(ev_manager);
}

Widgets::Panel* createRTXIPanel(QMainWindow* main_window,
                                Event::Manager* ev_manager)
{
  return new IIRfilter(main_window, ev_manager);
}

std::unique_ptr<Widgets::Component> createRTXIComponent(
    Widgets::Plugin* host_plugin)
{
  return std::make_unique<IIRfilterComponent>(host_plugin);
}

Widgets::FactoryMethods fact;

extern "C"
{
Widgets::FactoryMethods* getFactories()
{
  fact.createPanel = &createRTXIPanel;
  fact.createComponent = &createRTXIComponent;
  fact.createPlugin = &createRTXIPlugin;
  return &fact;
}
};

