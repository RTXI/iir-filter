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
* Creates IIR filters
* Butterworth: passband_edge
* Chebyshev: passband_ripple, passband_edge, ripple_bw_norm
* Elliptical: passband_ripple, stopband_ripple, passband_edge, stopband_edge
*/

#include <QComboBox>
#include <QFile>
#include <QTextStream>
#include <rtxi/dsp/iir_dsgn.h>
#include <rtxi/dsp/dir1_iir.h>
#include <rtxi/dsp/unq_iir.h>
#include <rtxi/dsp/buttfunc.h>
#include <rtxi/dsp/chebfunc.h>
#include <rtxi/dsp/elipfunc.h>
#include <rtxi/dsp/bilinear.h>
#include <rtxi/widgets.hpp>

#define TWO_PI 6.28318531

class IIRfilterComponent : public Widgets::Component{
	public:
		explicit IIRfilterComponent(Widgets::Plugin* host_plugin);
		void execute() override;
		std::vector<double> getNumeratorCoefficients();
		std::vector<double> getDenominatorCoefficients();
	private:
		enum filter_t : uint64_t {
			BUTTER=0, CHEBY, ELLIP,
		};
		// filter parameters
		FilterTransFunc *analog_filter;
		IirFilterDesign *filter_design;
		FilterImplementation *filter_implem;

		double h3; // filter coefficients
		filter_t filter_type; // type of filter
		double passband_ripple; // dB?
		double stopband_ripple; // dB?
		double passband_edge; // Hz
		double stopband_edge; // Hz
		int filter_order; // filter order
		int ripple_bw_norm; // type of normalization for Chebyshev filter

		bool quant_enabled; // quantize input signal and coefficients
		bool predistort_enabled; // predistort frequencies for bilinear transform
		int input_quan_factor; // quantization factor 2^bits for input signal
		int coeff_quan_factor; // quantization factor 2^bits for filter coefficients

		// bookkeeping
		double out; // bookkeeping for computing convolution
		int n; // bookkeeping for computing convolution
		double dt; // real-time period of system (s)

		// IIRfilter functions
		void initParameters();
		void makeFilter();
};

class IIRfilter : public Widgets::Panel {
	Q_OBJECT

	public:
		IIRfilter(QMainWindow* main_window, Event::Manager* ev_manager);
		void customizeGUI();
	
	private:	

		QComboBox *filterType;
		QComboBox *normType;

		// Saving FIR filter data to file without Data Recorder
		bool OpenFile(QString);
		QFile dataFile;
		QTextStream stream;

	private slots:
		// all custom slots
		void saveIIRData(); // write filter parameters to a file
		void updateFilterType(int);
		void updateNormType(int);
		void togglePredistort(bool);
		void toggleQuantize(bool);
};

class IIRfilterPlugin : public Widgets::Plugin
{
public:
	explicit IIRfilterPlugin(Event::Manager* ev_manager);
	std::vector<double> getIIRfilterNumeratorCoefficients();
	std::vector<double> getIIRfilterDenominatorCoefficients();
};

