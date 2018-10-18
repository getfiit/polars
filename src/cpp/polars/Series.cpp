//
// Created by Calvin Giles on 16/03/2018.
//

/**
 * Series
 *
 * This module defines a Series class that is inspired by a python pandas Series class.
 *
 *
 * Style guide when extending Series
 * =====================================
 * Where possible, this class will behave the same as a pandas Series. The supported features
 * of this class will be a subset of what is available by pandas.Series, and the typing system of C++ will be used to
 * full effect here where it makes sense.
 *
 */

#include "Series.h"

#include "SeriesMask.h"
#include "numc.h"


namespace polars {


    using SeriesMask = polars::SeriesMask;

    Series::Series() = default;


// todo; check for 1-D series & that the lengths match
    Series::Series(const arma::vec &v, const arma::vec &t) : t(t), v(v) {
        //assert(t.n_cols == 1 && v.n_cols == 1);
        //assert(t.n_rows == v.n_rows);
    };

    Series Series::from_vect(const std::vector<double> &t_v, const std::vector<double> &v_v) {
        return Series(arma::conv_to<arma::vec>::from(v_v), arma::conv_to<arma::vec>::from(t_v));
    }

    Series Series::from_map(const std::map<double, double> &iv_map) {
        arma::vec index(iv_map.size());
        arma::vec values(iv_map.size());
        int i = 0;
        for (auto& pair : iv_map) {
            index[i] = pair.first;
            values[i] = pair.second;
            ++i;
        }
        return {values, index};
    }


// Series [op] int methods
    SeriesMask Series::operator==(const int rhs) const {
        arma::vec rhs_vec = arma::ones(this->size()) * rhs;
        arma::vec abs_diff = arma::abs(values() - rhs_vec);
        // We can't use a large difference test like .1 despite the rhs is an int since the lhs is double so could be close.
        double threshold = 1E-50;
        return SeriesMask(abs_diff < threshold, index());
    }


    SeriesMask Series::operator!=(const int rhs) const {  // TODO implement as negation of operator==
        arma::vec rhs_vec = arma::ones(this->size()) * rhs;
        arma::vec abs_diff = arma::abs(values() - rhs_vec);
        // We can't use a large difference test like .1 despite the rhs is an int since the lhs is double so could be close.
        double threshold = 1E-50;
        return SeriesMask(abs_diff > threshold, index());
    }


    // Series [op] Series methods
    SeriesMask Series::operator==(const Series &rhs) const {
        // TODO: make this fast enough to always check at runtime
        //assert(!arma::any(index() != rhs.index()));  // Use not any != to handle empty array case
        return SeriesMask(values() == rhs.values(), index());
    }


    SeriesMask Series::operator!=(const Series &rhs) const {
        // TODO: make this fast enough to always check at runtime
        //assert(!arma::any(index() != rhs.index()));  // Use not any != to handle empty array case
        return SeriesMask(values() != rhs.values(), index());
    }


    SeriesMask Series::operator>(const Series &rhs) const {
        //assert(!arma::any(index() != rhs.index()));  // Use not any != to handle empty array case
        return SeriesMask(values() > rhs.values(), index());
    }


    SeriesMask Series::operator<(const Series &rhs) const {
        //assert(!arma::any(index() != rhs.index()));  // Use not any != to handle empty array case
        return polars::SeriesMask(values() < rhs.values(), index());
    }


    Series Series::operator+(const Series &rhs) const {
        //assert(!arma::any(index() != rhs.index()));  // Use not any != to handle empty array case
        return polars::Series(values() + rhs.values(), index());
    }


    Series Series::operator-(const Series &rhs) const {
        //assert(!arma::any(index() != rhs.index()));  // Use not any != to handle empty array case
        return polars::Series(values() - rhs.values(), index());
    }


    Series Series::operator*(const Series &rhs) const {
        //assert(!arma::any(index() != rhs.index()));  // Use not any != to handle empty array case
        return polars::Series(values() % rhs.values(), index());
    }


// Series [op] double methods
    SeriesMask Series::operator>(const double &rhs) const {
        return SeriesMask(values() > (arma::ones(size()) * rhs), index());
    }

    SeriesMask Series::operator>=(const double &rhs) const {
        return SeriesMask(values() >= (arma::ones(size()) * rhs), index());
    }

    SeriesMask Series::operator<=(const double &rhs) const {
        return SeriesMask(values() <= (arma::ones(size()) * rhs), index());
    }

    Series Series::operator+(const double &rhs) const {
        return Series(values() + rhs, index());
    }


    Series Series::operator-(const double &rhs) const {
        return Series(values() - rhs, index());
    }


    Series Series::operator*(const double &rhs) const {
        return Series(values() * rhs, index());
    }


// todo; do we need a flavor that *doesn't* take account of NANs?
    bool Series::equals(const Series &rhs) const {
        if ((index().n_rows != rhs.index().n_rows)) return false;
        if ((index().n_cols != rhs.index().n_cols)) return false;
        if (!polars::numc::equal_handling_nans(values(), rhs.values())) return false;
        if (any(index() != rhs.index())) return false;
        return true;
    }


// todo; do we need a flavor that *doesn't* take account of NANs?
    bool Series::almost_equals(const Series &rhs) const {
        if ((index().n_rows != rhs.index().n_rows)) return false;
        if ((index().n_cols != rhs.index().n_cols)) return false;
        if (!polars::numc::almost_equal_handling_nans(values(), rhs.values())) return false;
        if (any(index() != rhs.index())) return false;
        return true;
    }

// Location.

// by  position of the indices

    // TODO: Add slicing logic of the form .iloc(int start, int stop, int step=1) so it can be called like ser.iloc(0, -10).
    Series Series::iloc(const arma::uvec &pos) const {
        return Series(values().elem(pos), index().elem(pos));
    }


    double Series::iloc(arma::uword pos) const {
        arma::vec val = values().elem(arma::uvec{pos});
        return val[0];
    }

// by label of indices
    Series Series::loc(const arma::vec &index_labels) const {

        std::vector<int> indices;

        for (int j = 0; j < index_labels.n_elem; j++) {

            arma::uvec idx = arma::find(index() == index_labels[j]);

            if (!idx.empty()) {
                indices.push_back(idx[0]);
            }
        }

        if (indices.empty()) {
            return Series();
        } else {
            arma::uvec indices_v = arma::conv_to<arma::uvec>::from(indices);
            return Series(values().elem(indices_v), index().elem(indices_v));
        }
    }

    Series Series::loc(arma::uword pos) const {
        arma::uvec idx = arma::find(index() == pos);

        if (!idx.empty()) {
            return Series(values(), index()).iloc(idx);
        } else {
            return Series();
        }
    }

    Series Series::where(const SeriesMask &condition, double other) const {
        arma::vec result = values();
        result.elem(find((!condition).values())).fill(other);
        return Series(result, index());
    }


    Series Series::diff() const {

        arma::uword resultSize = values().size();

        arma::vec resultv(resultSize);

        double previousValue = NAN;
        for (arma::uword idx = 0; idx < resultSize; idx++) {
            resultv[idx] = values()[idx] - previousValue;
            previousValue = values()[idx];
        }

        return Series(resultv, index());
    }


    Series Series::abs() const {
        return Series(arma::abs(values()), index());
    }

    double Series::quantile(double q) const {
        return polars::numc::quantile(values(), q);
    }

    Series Series::fillna(double value) const {
        arma::vec vals = values();
        vals.replace(arma::datum::nan, value);
        return Series(vals, index());
    }

    Series Series::dropna() const {
        // Get indices of finite elements
        arma::uvec indices = arma::sort(arma::join_cols(
                arma::find_finite(values()),
                arma::find(arma::abs(values()) == arma::datum::inf))
        );
        return Series(values().elem(indices), index().elem(indices));
    }


    arma::vec calculate_window_weights(
            polars::WindowProcessor::WindowType win_type,
            arma::uword windowSize,
            double alpha
    ) {
        switch (win_type) {
            case (polars::WindowProcessor::WindowType::none):
                return arma::ones(windowSize);
            case (polars::WindowProcessor::WindowType::triang):
                return polars::numc::triang(windowSize);
            case (polars::WindowProcessor::WindowType::expn):
                return reverse(polars::numc::exponential(windowSize, -1. / log(1 - alpha), false, 0));
            default:
                return arma::ones(windowSize);
        }
    }


    arma::vec _ewm_correction(const arma::vec &results, const arma::vec &vals,
                                      polars::WindowProcessor::WindowType win_type) {
        /* Method that shifts result from rolling average with exp window so it yields correct normalisation and allows usage
         * of rolling method hereby implemented.
         * This matches pandas ewm for its default case */

        if (results.n_elem == 0) {
            return arma::vec({});
        }

        if (win_type == polars::WindowProcessor::WindowType::expn) {
            // Correction to match pandas ewm - shift by one.
            arma::vec results_ewm;
            results_ewm.copy_size(results);
            results_ewm.at(0) = vals[0];

            for (int j = 1; j < results_ewm.n_elem; j++) {
                results_ewm[j] = results[j - 1];
            }

            return results_ewm;

        } else {
            return results;
        }
    }


// todo; allow passing in transformation function rather than WindowProcessor.
    Series
    Series::rolling(SeriesSize windowSize, const polars::WindowProcessor &processor, SeriesSize minPeriods,
                    bool center, bool symmetric, polars::WindowProcessor::WindowType win_type, double alpha) const {

        //assert(center); // todo; implement center:false
        //assert(windowSize > 0);
        //assert(windowSize % 2 == 0); // TODO: Make symmetric = true work for even windows. See tests for reference.

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
            arma::sword weightLeftIdx = 0;
            arma::sword weightRightIdx = windowSize - 1;

            if (symmetric) {
                // This option works for odd windows only.
                if (leftIdx < 0) {
                    arma::sword left_err = leftIdx;
                    arma::sword right_err = windowSize - 1 - centerIdx - centerOffset;

                    weightLeftIdx = weightLeftIdx - left_err;
                    weightRightIdx = weightRightIdx - right_err;

                    rightIdx = rightIdx - right_err;
                    leftIdx = leftIdx - left_err;
                }

                if (rightIdx >= size()) {
                    arma::sword r_clipped = rightIdx - size();

                    arma::sword left_err = -r_clipped - 1;
                    arma::sword right_err = rightIdx - (size() - 1);

                    weightLeftIdx = weightLeftIdx - left_err;
                    weightRightIdx = weightRightIdx - right_err;

                    leftIdx = leftIdx - left_err;
                    rightIdx = rightIdx - right_err;
                }

            } else {

                if (leftIdx < 0) {
                    arma::sword left_err = leftIdx;

                    weightLeftIdx = weightLeftIdx - left_err;
                    leftIdx = leftIdx - left_err;
                }
                if (rightIdx >= size()) {
                    arma::sword right_err = rightIdx - (size() - 1);

                    weightRightIdx = weightRightIdx - right_err;
                    rightIdx = rightIdx - right_err;
                }
            }


            arma::vec values = v.subvec(leftIdx, rightIdx);

            // Define weights vector required for specific windows
            arma::vec weights;
            weights.copy_size(values);
            weights = polars::calculate_window_weights(win_type, windowSize, alpha).subvec(weightLeftIdx,
                                                                                           weightRightIdx);

            const Series subSeries = Series(values, t.subvec(leftIdx, rightIdx));

            if (subSeries.finiteSize() >= minPeriods) {
                resultv(centerIdx) = processor.processWindow(subSeries, weights);
            } else {
                resultv(centerIdx) = processor.defaultValue();
            }
        }

        return Series(polars::_ewm_correction(resultv, v, win_type), t);

    }

    Window Series::rolling(SeriesSize windowSize,
                   SeriesSize minPeriods,
                   bool center,
                   bool symmetric,
                   polars::WindowProcessor::WindowType win_type,
                   double alpha) const {
        return Window((*this), windowSize, minPeriods, center, symmetric, win_type, alpha);
    };

    Rolling Series::rolling(SeriesSize windowSize,
                    SeriesSize minPeriods,
                    bool center,
                    bool symmetric) const {
        return Rolling((*this), windowSize, minPeriods, center, symmetric);
    };


    Series Series::clip(double lower_limit, double upper_limit) const {
        SeriesMask upper = SeriesMask(values() < upper_limit, index());
        SeriesMask lower = SeriesMask(values() > lower_limit, index());
        return Series(values(), index()).where(upper, upper_limit).where(lower, lower_limit);
    };


    Series Series::pow(double power) const {
        return Series(arma::pow(values(), power), index());
    }


    int Series::count() const {
        return finiteSize();
    }


    double Series::sum() const {
        arma::vec finites = finiteValues();
        if (finites.size() == 0) {
            return NAN;
        } else {
            return arma::sum(finites);
        }
    }


    double Series::mean() const {
        arma::vec finites = finiteValues();
        if (finites.size() == 0) {
            return NAN;
        } else {
            return arma::mean(finites);
        }
    }


    double Series::std(int ddof) const {
        arma::vec finites = finiteValues();
        if (ddof < 0) {
            ddof = 0;
        }
        auto n = finites.size();
        if (n <= ddof) {
            return NAN;
        } else {
            auto dev = (*this) - this->mean();
            auto squared_deviation = dev.pow(2);
            return std::pow(squared_deviation.sum() / (n - ddof), 0.5);
        }
    }


    Series::SeriesSize Series::size() const {
        //assert(index().size() == values().size());
        return index().size();
    }


    arma::vec Series::finiteValues() const {
        return values().elem(find_finite(values()));
    }


    Series::SeriesSize Series::finiteSize() const {
        //assert(index().size() == values().size());
        return finiteValues().size();
    }


// done this way so default copy / assignment works.
// todo; make copies of indices share memory as they are const?
    const arma::vec Series::index() const {
        return t;
    }


    const arma::vec Series::values() const {
        return v;
    }


    bool Series::equal(const Series &lhs, const Series &rhs) {
        return lhs.equals(rhs);
    }


    bool Series::almost_equal(const Series &lhs, const Series &rhs) {
        return lhs.almost_equals(rhs);
    }


    bool Series::not_equal(const Series &lhs, const Series &rhs) {
        return !lhs.equals(rhs);
    }

    Series Series::apply(double (*f)(double)) const {
        arma::vec vals = values();
        vals.transform([=](double val) { return (f(val)); });
        return Series(vals, index());
    }

    Series Series::index_as_series() const {
        return Series(index(), index());
    }

    std::map<double, double> Series::to_map() const {

        std::map<double, double> m;
        // put pairs into map
        for (int i = 0; i < size(); i++) {
            m.insert(std::make_pair(index()[i], values()[i]));
        }

        return m;
    }

    bool Series::empty() const {
        return (index().is_empty() & values().is_empty());
    }

    // TODO: Modify head once iloc has been refactored to accept slicing logic.
    Series Series::head(int n) const  {
        Series ser(values(), index());
        if(n >= ser.size()){
            return ser;
        } else {
            arma::uvec indices = arma::conv_to<arma::uvec>::from(polars::numc::arange(0, n));
            return ser.iloc(indices);
        }
    }

    // TODO: Modify tail once iloc has been refactored to accept slicing logic.
    Series Series::tail(int n) const  {

        Series ser(values(), index());

        if(n >= ser.size()){
            return ser;
        } else {
            arma::uword l = ser.size() - n;
            arma::uvec indices = arma::conv_to<arma::uvec>::from(polars::numc::arange(l, ser.size()));
            return ser.iloc(indices);
        }
    }

    /**
     * Add support for pretty printing of a Series object.
     * @param os the output stream that will be written to
     * @param ts the Series instance to output
     * @return the ostream for further piping
     */
    std::ostream &operator<<(std::ostream &os, const Series &ts) {

        if(ts.size() >= 5){
            os << "Series:\nindices\n" << ts.head(5).index() << "values\n" << ts.head(5).values();
            os << "\n....\n";
            os << "Series:\nindices\n" << ts.tail(5).index() << "values\n" << ts.tail(5).values();
            return os;
        } else {
            return os << "Series:\nindices\n" << ts.index() << "values\n" << ts.values();
        }
    }
}; // polars
