/* Tables for CO2 fluid properties calculated according to Span and
 * Wagner (1996) and using a web service of the National Institute
 * of Standards and Techlology of the U.S. Department of Commerce:
 * https://webbook.nist.gov/
 *
 * THIS AN AUTO-GENERATED FILE! DO NOT EDIT IT!
 *
 * Generated using:
 *
 * python make_co2_table.py  using:
 * MIN_TEMP = $MIN_TEMP;
 * MAX_TEMP = $MAX_TEMP;
 * NUM_TEMP_SAMPLES = $NUM_TEMP_SAMPLES;
 * MIN_PRESS = $MIN_PRESS;
 * MAX_PRESS = $MAX_PRESS;
 * NUM_PRESS_SAMPLES = $NUM_PRESS_SAMPLES;
 */


struct TabulatedDensityTraits {
    typedef double Scalar;
    static const char  *name;
    static const int    numTempSteps = $NUM_TEMP_SAMPLES;
    static constexpr Scalar minTemp = $MIN_TEMP;
    static constexpr Scalar maxTemp = $MAX_TEMP;
    static const int    numPressSteps = $NUM_PRESS_SAMPLES;
    static constexpr Scalar minPress = $MIN_PRESS;
    static constexpr Scalar maxPress = $MAX_PRESS;
    static const Scalar vals[numTempSteps][numPressSteps];
};

const char  *TabulatedDensityTraits::name = "density";

const double TabulatedDensityTraits::vals[$NUM_TEMP_SAMPLES][$NUM_PRESS_SAMPLES] =
{
$DENSITY_VALS
};

typedef TabulatedCO2Properties< TabulatedDensityTraits > TabulatedDensity;

struct TabulatedEnthalpyTraits {
    typedef double Scalar;
    static const char  *name;
    static const int    numTempSteps = $NUM_TEMP_SAMPLES;
    static constexpr Scalar minTemp = $MIN_TEMP;
    static constexpr Scalar maxTemp = $MAX_TEMP;
    static const int    numPressSteps = $NUM_PRESS_SAMPLES;
    static constexpr Scalar minPress = $MIN_PRESS;
    static constexpr Scalar maxPress = $MAX_PRESS;
    static const Scalar vals[numTempSteps][numPressSteps];
};

const char  *TabulatedEnthalpyTraits::name = "enthalpy";

const double TabulatedEnthalpyTraits::vals[$NUM_TEMP_SAMPLES][$NUM_PRESS_SAMPLES] =
{
$ENTHALPY_VALS
};

typedef TabulatedCO2Properties< TabulatedEnthalpyTraits > TabulatedEnthalpy;


// this class collects all the tabulated quantities in one convenient place
struct CO2Tables {
   static const TabulatedEnthalpy   tabulatedEnthalpy;
   static const TabulatedDensity    tabulatedDensity;
};

const TabulatedEnthalpy CO2Tables::tabulatedEnthalpy;
const TabulatedDensity CO2Tables::tabulatedDensity;

