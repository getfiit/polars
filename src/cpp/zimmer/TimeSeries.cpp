//
// Created by Calvin Giles on 16/03/2018.
//

/**
 * TimeSeries
 *
 * This module defines a TimeSeries class that is inspired by a python pandas Series class.
 *
 *
 * Style guide when extending TimeSeries
 * =====================================
 * Where possible, this class will behave the same as a pandas Series with a DataTimeIndex would. The supported features
 * of this class will be a subset of what is available by pandas.Series, and the typing system of C++ will be used to
 * full effect here where it makes sense.
 *
 */

#include "TimeSeriesMask.h"
#include "TimeSeries.h"
#include "numc.h"

typedef zimmer::TimeSeriesMask TimeSeriesMask;
namespace zimmer {


// check if a double is an integer value, e.g. 2.0 vs 2.1
    bool double_is_int(double v) {
        double intpart;
        return modf(v, &intpart) == 0;
    }


    Quantile::Quantile(double quantile) : quantile(quantile) {
        //assert(quantile >= 0);
        //assert(quantile <= 1);
    }


    double Quantile::processWindow(const TimeSeries &window, const WindowType win_type) const {
        arma::vec v = sort(window.finiteValues());
        // note, this is based on how q works in python numpy percentile rather than the more usual quantile defn.
        double quantilePosition = quantile * ((double) v.size() - 1);
        if (double_is_int(quantilePosition)) {
            return v(quantilePosition);
        } else {
            // interpolate estimate
            arma::uword quantileIdx = floor(quantilePosition);
            double fraction = quantilePosition - quantileIdx;
            return v(quantileIdx) + (v(quantileIdx + 1) - v(quantileIdx)) * fraction;
        }
    }


    double zimmer::Sum::processWindow(const TimeSeries &window, const WindowType win_type) const {
        return arma::sum(window.finiteValues());
    }


    zimmer::Count::Count(double default_value) : default_value(default_value) {}


    double zimmer::Count::processWindow(const TimeSeries &window, const WindowType win_type) const {
        return window.finiteSize();
    }


    zimmer::Mean::Mean(double default_value) : default_value(default_value) {}


    double zimmer::Mean::processWindow(const TimeSeries &window, const WindowType win_type) const {

        if(win_type == WindowType::triang){
            auto weight = arma::sum(zimmer::numc::triang(window.size()));
            return arma::sum(window.finiteValues()) / weight;
        } else {
            return arma::sum(window.finiteValues()) / window.finiteSize();
        }
    }

} // zimmer

TimeSeries::TimeSeries() = default;


// todo; check for 1-D series & that the lengths match
TimeSeries::TimeSeries(const arma::vec &t, const arma::vec &v) : t(t), v(v) {
    //assert(t.n_cols == 1 && v.n_cols == 1);
    //assert(t.n_rows == v.n_rows);
};


// TimeSeries [op] int methods
TimeSeriesMask TimeSeries::operator==(const int rhs) const {
    arma::vec rhs_vec = arma::ones(this->size()) * rhs;
    arma::vec abs_diff = arma::abs(values() - rhs_vec);
    // We can't use a large difference test like .1 despite the rhs is an int since the lhs is double so could be close.
    double threshold = 1E-50;
    return TimeSeriesMask(timestamps(), abs_diff < threshold);
}


TimeSeriesMask TimeSeries::operator!=(const int rhs) const {  // TODO implement as negation of operator==
    arma::vec rhs_vec = arma::ones(this->size()) * rhs;
    arma::vec abs_diff = arma::abs(values() - rhs_vec);
    // We can't use a large difference test like .1 despite the rhs is an int since the lhs is double so could be close.
    double threshold = 1E-50;
    return TimeSeriesMask(timestamps(), abs_diff > threshold);
}


// TimeSeries [op] TimeSeries methods
TimeSeriesMask TimeSeries::operator==(const TimeSeries &rhs) const {
    // TODO: make this fast enough to always check at runtime
    //assert(!arma::any(timestamps() != rhs.timestamps()));  // Use not any != to handle empty array case
    return TimeSeriesMask(timestamps(), values() == rhs.values());
}


TimeSeriesMask TimeSeries::operator>(const TimeSeries &rhs) const {
    //assert(!arma::any(timestamps() != rhs.timestamps()));  // Use not any != to handle empty array case
    return TimeSeriesMask(timestamps(), values() > rhs.values());
}


TimeSeriesMask TimeSeries::operator<(const TimeSeries &rhs) const {
    //assert(!arma::any(timestamps() != rhs.timestamps()));  // Use not any != to handle empty array case
    return zimmer::TimeSeriesMask(timestamps(), values() < rhs.values());
}


TimeSeries TimeSeries::operator+(const TimeSeries &rhs) const {
    //assert(!arma::any(timestamps() != rhs.timestamps()));  // Use not any != to handle empty array case
    return zimmer::TimeSeries(timestamps(), values() + rhs.values());
}


TimeSeries TimeSeries::operator-(const TimeSeries &rhs) const {
    //assert(!arma::any(timestamps() != rhs.timestamps()));  // Use not any != to handle empty array case
    return zimmer::TimeSeries(timestamps(), values() - rhs.values());
}


TimeSeries TimeSeries::operator*(const TimeSeries &rhs) const {
    //assert(!arma::any(timestamps() != rhs.timestamps()));  // Use not any != to handle empty array case
    return zimmer::TimeSeries(timestamps(), values() % rhs.values());
}


// TimeSeries [op] double methods
TimeSeriesMask TimeSeries::operator>(const double &rhs) const {
    return TimeSeriesMask(timestamps(), values() > (arma::ones(size()) * rhs));
}

TimeSeriesMask TimeSeries::operator>=(const double &rhs) const {
    return TimeSeriesMask(timestamps(), values() >= (arma::ones(size()) * rhs));
}

TimeSeriesMask TimeSeries::operator<=(const double &rhs) const {
    return TimeSeriesMask(timestamps(), values() <= (arma::ones(size()) * rhs));
}

TimeSeries TimeSeries::operator+(const double &rhs) const {
    return TimeSeries(timestamps(), values() + rhs);
}


TimeSeries TimeSeries::operator-(const double &rhs) const {
    return TimeSeries(timestamps(), values() - rhs);
}


TimeSeries TimeSeries::operator*(const double &rhs) const {
    return TimeSeries(timestamps(), values() * rhs);
}


// todo; do we need a flavor that *doesn't* take account of NANs?
bool TimeSeries::equals(const TimeSeries &rhs) const {
    if ((timestamps().n_rows != rhs.timestamps().n_rows)) return false;
    if ((timestamps().n_cols != rhs.timestamps().n_cols)) return false;
    if (!zimmer::numc::equal_handling_nans(values(), rhs.values())) return false;
    if (any(timestamps() != rhs.timestamps())) return false;
    return true;
}


// todo; do we need a flavor that *doesn't* take account of NANs?
bool TimeSeries::almost_equals(const TimeSeries &rhs) const {
    if ((timestamps().n_rows != rhs.timestamps().n_rows)) return false;
    if ((timestamps().n_cols != rhs.timestamps().n_cols)) return false;
    if (!zimmer::numc::almost_equal_handling_nans(values(), rhs.values())) return false;
    if (any(timestamps() != rhs.timestamps())) return false;
    return true;
}


TimeSeries TimeSeries::where(const TimeSeriesMask &condition, double other) const {
    arma::vec result = values();
    result.elem(find((!condition).values())).fill(other);
    return TimeSeries(timestamps(), result);
}


TimeSeries TimeSeries::diff() const {

    arma::uword resultSize = values().size();

    arma::vec resultv(resultSize);

    double previousValue = NAN;
    for (arma::uword idx = 0; idx < resultSize; idx++) {
        resultv[idx] = values()[idx] - previousValue;
        previousValue = values()[idx];
    }

    return TimeSeries(timestamps(), resultv);
}


TimeSeries TimeSeries::abs() const {
    return TimeSeries(timestamps(), arma::abs(values()));
}


// todo; allow passing in transformation function rather than WindowProcessor.
TimeSeries
TimeSeries::rolling(SeriesSize windowSize, const zimmer::WindowProcessor &processor, SeriesSize minPeriods,
                    bool center, bool symmetric, zimmer::WindowProcessor::WindowType win_type) const {

    //assert(center); // todo; implement center:false
    //assert(windowSize > 0);
    //assert(windowSize % 2 == 0); // TODO: Make symmetric = true and even windows work!

    if (minPeriods == 0) {
        minPeriods = windowSize;
    }

    arma::uword resultSize = size();
    arma::vec resultv(resultSize);

    arma::uword centerOffset = round(((float) windowSize - 1) / 2.0);

    // roll a window [left,right], of up to size windowSize, centered on centerIdx, and hand to processor if there are minPeriods finite values.
    for (arma::uword centerIdx = 0; centerIdx < size(); centerIdx++) {

        arma::sword leftIdx = centerIdx - centerOffset;
        arma::sword rightIdx = centerIdx - centerOffset + windowSize - 1;

        if (symmetric) {
            // Estimate windows available:
            if (leftIdx < 0){
                rightIdx = centerIdx + centerOffset + leftIdx;
                leftIdx = 0;
            }

            if (rightIdx >= size()) {
                arma::sword r_clipped = rightIdx - size();
                leftIdx = leftIdx + r_clipped + 1;
                rightIdx = size() - 1;
            }

        } else {

            if (leftIdx < 0) {
                leftIdx = 0;
            }
            if (rightIdx >= size()) {
                rightIdx = size() - 1;
            }
        }

        arma::vec values = v.subvec(leftIdx, rightIdx);

        if(win_type == zimmer::WindowProcessor::WindowType::triang){

            auto weights = zimmer::numc::triang(windowSize);

            for (int i = 0; i < values.size(); i += 1){
                // When at edges, size of weights and values not the same so can't use % operator
                values[i] = weights.at(i) * values[i];
            }
        }

        const TimeSeries subSeries = TimeSeries(t.subvec(leftIdx, rightIdx), values);

        if (subSeries.finiteSize() >= minPeriods) {
            std::vector<double> ext = arma::conv_to< std::vector<double> >::from(subSeries.values());

            resultv(centerIdx) = processor.processWindow(subSeries, win_type);
        } else {
            resultv(centerIdx) = processor.defaultValue();
        }
    }

    return TimeSeries(t, resultv);
}


TimeSeries TimeSeries::pow(double power) const {
    return TimeSeries(timestamps(), arma::pow(values(), power));
}


double TimeSeries::mean() const {
    arma::vec finites = finiteValues();
    if (finites.size() == 0) {
        return NAN;
    } else {
        return arma::mean(finites);
    }
}


TimeSeries::SeriesSize TimeSeries::size() const {
    //assert(timestamps().size() == values().size());
    return timestamps().size();
}


arma::vec TimeSeries::finiteValues() const {
    return values().elem(find_finite(values()));
}


TimeSeries::SeriesSize TimeSeries::finiteSize() const {
    //assert(timestamps().size() == values().size());
    return finiteValues().size();
}


// done this way so default copy / assignment works.
// todo; make copies of timeseries share memory as they are const?
const arma::vec TimeSeries::timestamps() const {
    return t;
}


const arma::vec TimeSeries::values() const {
    return v;
}


bool TimeSeries::equal(const TimeSeries &lhs, const TimeSeries &rhs) {
    return lhs.equals(rhs);
}


bool TimeSeries::almost_equal(const TimeSeries &lhs, const TimeSeries &rhs) {
    return lhs.almost_equals(rhs);
}


bool TimeSeries::not_equal(const TimeSeries &lhs, const TimeSeries &rhs) {
    return !lhs.equals(rhs);
}


/**
 * Add support for pretty printing of a TimeSeries object.
 * @param os the output stream that will be written to
 * @param ts the TimeSeries instance to output
 * @return the ostream for further piping
 */
std::ostream &operator<<(std::ostream &os, const TimeSeries &ts) {
    return os << "TimeSeries:\ntimestamps\n" << ts.timestamps() << "values\n" << ts.values();
}
