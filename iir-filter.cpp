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

#include "iir-filter.h"
#include <math.h>
#include <algorithm>
#include <numeric>
#include <time.h>
#include <DSP/log2.h>
#include <sys/stat.h>

//create plug-in
extern "C" Plugin::Object *createRTXIPlugin(void){
	return new IIRfilter(); // Change the name of the plug-in here
}

//set up parameters/inputs/outputs derived from defaultGUIModel, calls for initialization, creation, update, and refresh of GUI
static DefaultGUIModel::variable_t vars[] = {
	{ "Input", "Input to Filter", DefaultGUIModel::INPUT, },
	{ "Output", "Output of Filter", DefaultGUIModel::OUTPUT },
	{ "Filter Order", "Filter Order", DefaultGUIModel::PARAMETER
		| DefaultGUIModel::INTEGER, },
	{ "Passband Ripple (dB)", "Passband Ripple (dB)", DefaultGUIModel::PARAMETER
		| DefaultGUIModel::DOUBLE, },
	{ "Passband Edge (Hz)", "Passband Edge (Hz)", DefaultGUIModel::PARAMETER
		| DefaultGUIModel::DOUBLE, },
	{ "Stopband Ripple (dB)", "Stopband Ripple (dB)", DefaultGUIModel::PARAMETER
		| DefaultGUIModel::DOUBLE, },
	{ "Stopband Edge (Hz)", "Stopband Edge (Hz)", DefaultGUIModel::PARAMETER
		| DefaultGUIModel::DOUBLE, },
	{ "Input quantizing factor", "Bits eg. 10, 12, 16",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::INTEGER, },
	{ "Coefficients quantizing factor", "Bits eg. 10, 12, 16",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::INTEGER, },
};

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

IIRfilter::IIRfilter(void) : DefaultGUIModel("IIR Filter", ::vars, ::num_vars) {
	setWhatsThis(
	"<p><b>IIR Filter:</b><br>This plugin computes filter coefficients for three types of IIR filters. "
	"They require the following parameters: <br><br>"
	"Butterworth: passband edge <br>"
	"Chebyshev: passband ripple, passband edge, ripple bw_norm <br>"
	"Elliptical: passband ripple, stopband ripple, passband edge, stopband edge <br><br>"
	"Since this plug-in computes new filter coefficients whenever you change the parameters, you should not"
	"change any settings during real-time.</p>");
	
	initParameters();
	DefaultGUIModel::createGUI(vars, num_vars);
	customizeGUI();
	update(INIT);
	refresh(); // refresh the GUI
	QTimer::singleShot(0, this, SLOT(resizeMe()));
}

IIRfilter::~IIRfilter(void) {}

//execute, the code block that actually does the signal processing
void IIRfilter::execute(void) {
	output(0) = filter_implem->ProcessSample(input(0));
	return;
}

void IIRfilter::update(DefaultGUIModel::update_flags_t flag) {
	switch (flag) {
		case INIT:
			setParameter("Filter Order", QString::number(filter_order));
			setParameter("Passband Ripple (dB)", QString::number(passband_ripple));
			setParameter("Passband Edge (Hz)", QString::number(passband_edge));
			setParameter("Stopband Ripple (dB)", QString::number(stopband_ripple));
			setParameter("Stopband Edge (Hz)", QString::number(stopband_edge));
			setParameter("Input quantizing factor", QString::number(ilog2(input_quan_factor)));
			setParameter("Coefficients quantizing factor", QString::number(ilog2(coeff_quan_factor)));
			filterType->setCurrentIndex(filter_type);
			break;
	
		case MODIFY:
			filter_order = int(getParameter("Filter Order").toDouble());
			passband_ripple = getParameter("Passband Ripple (dB)").toDouble();
			passband_edge = getParameter("Passband Edge (Hz)").toDouble();
			stopband_ripple = getParameter("Stopband Ripple (dB)").toDouble();
			stopband_edge = getParameter("Stopband Edge (Hz)").toDouble();
			filter_type = filter_t(filterType->currentIndex());
			stopband_edge *= TWO_PI;
			input_quan_factor = 2 ^ getParameter("Input quantizing factor").toInt(); // quantize input to 12 bits
			coeff_quan_factor = 2 ^ getParameter("Coefficients quantizing factor").toInt(); // quantize filter coefficients to 12 bits
			makeFilter();
			break;
	
		case PAUSE:
			output(0) = 0; // stop command in case pause occurs in the middle of command
			break;
	
		case UNPAUSE:
			break;
	
		case PERIOD:
			dt = RT::System::getInstance()->getPeriod() * 1e-9; // s
			break;

		default:
			break;
	}
}

// custom functions, as defined in the header file

void IIRfilter::initParameters() {
	dt = RT::System::getInstance()->getPeriod() * 1e-9; // s
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
	if (index == 0) {
		filter_type = BUTTER;
		normType->setEnabled(false);
		makeFilter();
	} else if (index == 1) {
		filter_type = CHEBY;
		normType->setEnabled(true);
		makeFilter();
	} else if (index == 2) {
		filter_type = ELLIP;
		normType->setEnabled(false);
		makeFilter();
	}
}

void IIRfilter::updateNormType(int index) {
	ripple_bw_norm = index;
	makeFilter();
}

void IIRfilter::makeFilter() {
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
	
	filter_design = BilinearTransf(analog_filter, dt);
	
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
	if (fd->exec() == QDialog::Accepted) {
		QStringList files = fd->selectedFiles();
		if (!files.isEmpty()) fileName = files.takeFirst();
		
		if (OpenFile(fileName)) {
			// stream.setPrintableData(true);
			switch (filter_type) {
				case BUTTER:
					stream << QString("BUTTERWORTH order=") << (int) filter_order
						<< " passband edge=" << (double) passband_edge;
					break;

				case CHEBY:
					stream << QString("CHEBYSHEV order=") << (int) filter_order
						<< " passband ripple=" << (double) passband_ripple
						<< " passband edge=" << (double) passband_edge;

					if (ripple_bw_norm == 0) stream << " with 3 dB bandwidth normalization";
					else stream << " with ripple bandwidth normalization";
					break;
		
				case ELLIP:
					stream << QString("ELLIPTICAL order=") << (int) filter_order
						<< " passband ripple=" << (double) passband_ripple
						<< " passband edge=" << (double) passband_edge
						<< " stopband ripple=" << (double) stopband_ripple
						<< " stopband edge=" << (double) stopband_edge;
					break;
			}
			stream << QString(" \n");
			
			double *numer_coeff = filter_design->GetNumerCoefficients();
			double *denom_coeff = filter_design->GetDenomCoefficients();
			
			stream << QString("Filter numerator coefficients:\n");
			for (int i = 0; i < filter_design->GetNumNumerCoeffs(); i++) {
				stream << QString("numer_coeff[") << i << "] = "
					<< (double) numer_coeff[i] << "\n";
			}
			stream << QString("Filter denominator coefficients:\n");
			for (int i = 0; i < filter_design->GetNumDenomCoeffs() + 1; i++) {
				stream << QString("denom_coeff[") << i << "] = "
					<< (double) denom_coeff[i] << "\n";
			}
			dataFile.close();
			
			delete[] numer_coeff;
			delete[] denom_coeff;
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
	QGridLayout *customLayout = DefaultGUIModel::getLayout();

	QVBoxLayout *customGUILayout = new QVBoxLayout;
	
	QPushButton *saveDataButton = new QPushButton("Save IIR Coefficients");
	customGUILayout->addWidget(saveDataButton);
	QObject::connect(saveDataButton, SIGNAL(clicked()), this, SLOT(saveIIRData()));
	saveDataButton->setToolTip("Save filter parameters and coefficients to a file");
	
	QFormLayout *optionLayout = new QFormLayout;
	customGUILayout->addLayout(optionLayout);
	
	filterType = new QComboBox;
	filterType->setToolTip("IIR filter.");
	filterType->insertItem(1, "Butterworth");
	filterType->insertItem(2, "Chebyshev");
	filterType->insertItem(3," Elliptical");
	optionLayout->addRow("IIR filter", filterType);
	QObject::connect(filterType,SIGNAL(activated(int)), this, SLOT(updateFilterType(int)));
	
	normType = new QComboBox;
	normType->insertItem(1, "3 dB bandwidth");
	normType->insertItem(2, "Ripple bandwidth");
	normType->setToolTip("Type of Chebyshev normalization");
	optionLayout->addRow("Chebyshev Normalize Type:", normType);
	QObject::connect(normType,SIGNAL(activated(int)), this, SLOT(updateNormType(int)));
	normType->setEnabled(false);
	
	QFormLayout *checkBoxLayout = new QFormLayout;
	QCheckBox *predistortCheckBox = new QCheckBox;
	checkBoxLayout->addRow("Predistort frequencies", predistortCheckBox);
	QCheckBox *quantizeCheckBox = new QCheckBox;
	checkBoxLayout->addRow("Quantize input and coefficients", quantizeCheckBox);
	QObject::connect(predistortCheckBox,SIGNAL(toggled(bool)),this,SLOT(togglePredistort(bool)));
	QObject::connect(quantizeCheckBox,SIGNAL(toggled(bool)),this,SLOT(toggleQuantize(bool)));
	predistortCheckBox->setToolTip("Predistort frequencies for bilinear transform");
	quantizeCheckBox->setToolTip("Quantize input and coefficients");
	
	QObject::connect(DefaultGUIModel::pauseButton, SIGNAL(toggled(bool)), saveDataButton, SLOT(setEnabled(bool)));
	QObject::connect(DefaultGUIModel::pauseButton, SIGNAL(toggled(bool)), modifyButton, SLOT(setEnabled(bool)));
	DefaultGUIModel::pauseButton->setToolTip("Start/Stop filter");
	DefaultGUIModel::modifyButton->setToolTip("Commit changes to parameter values");
	DefaultGUIModel::unloadButton->setToolTip("Close plug-in");
	
	customLayout->addLayout(customGUILayout, 0, 0);
	customLayout->addLayout(checkBoxLayout, 2, 0);
	setLayout(customLayout);	
}

void IIRfilter::togglePredistort(bool on) {
	predistort_enabled = on;
}

void IIRfilter::toggleQuantize(bool on) {
	quant_enabled = on;
}
