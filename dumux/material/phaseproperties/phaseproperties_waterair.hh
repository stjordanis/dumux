// $Id$

#ifndef PHASEPROPERTIES_WATERAIR_HH_
#define PHASEPROPERTIES_WATERAIR_HH_

#include <dumux/material/property_baseclasses.hh>
#include <dumux/material/constrel/constrelair.hh>
#include <dumux/material/constrel/constrelwater.hh>
#include <math.h>

/**
 * \ingroup properties
 * \author Jochen Fritz
 */

namespace Dune
{

/**\ingroup properties
 * @brief property class for gaseous phase of air/water mixture
 */
class Gas_WaterAir : public Gas_GL
{
public:
	virtual double density(double T, double p, double Xw=0.) const // [kg / m^3]
	{
		double Rsm = R * (Xw / M_w + (1-Xw) / M_a); // medium specific gas constant
		return p / Rsm / T;
	}

	virtual double viscosity(double T, double p, double Xw=0.) const // [kg / (m*s)]
	{
		double v_a = constRelAir.viscosity_air(T); // see constrelair.hh
		double v_w = constRelAir.visco_w_vap(T);    // see constrelair.hh
		FieldVector<double,2> X(Xw); X[1] = (1-Xw);
		X = X2x(X);
		X[0] *= sqrt(M_w);
		X[1] *= sqrt(M_a);
		return (v_w * X[0] + v_a * X[1]) / (X[0] + X[1]); // after Herning & Zipperer, 1936
	}

	virtual double intEnergy(double T, double p, double Xw=0.) const
	{
		return enthalpy(T,p,Xw) - p/density(T,p,Xw);
	}

	virtual double enthalpy(double T, double p, double Xw=0.) const
	{
		double H_a = 1005 * (T - 273.15);
		double H_w;
		H_w = constRelWater.enthalpy_water(T,p);
//		if (T < 273.15)
//			H_w = constRelWater.sp_enthalpy_IAPWS2(273.15, p) + 4000 * (T - 273.15);
//		else
//			H_w = constRelWater.sp_enthalpy_IAPWS2(T, p);
		return Xw * H_w + (1-Xw) * H_a;
	}

	virtual double diffCoeff(double T, double p) const
	{
		// D ~ T^(3/2) / see Atkins:Physical Chemistry p.778!
		// for H2O and O2: D(273.15 K, 1e5 Pa) = 2.25 e-5
		return 2.25e-5 * pow(T/273.15, 2/3) * 1e5 / p;
	}

	virtual double Xw_Max(double T, double p) const
	{
		double pwsat = constRelAir.pwsat(T);
		FieldVector<double,2> x(std::min(pwsat / p, 1.)); x[1] = 1-x[0];
		x = x2X(x);
		return x[0];
	}

	Gas_WaterAir() : Gas_GL()
	{
		M_w = 0.018016;
		M_a = 0.02896;
	}

private:
	ConstrelAir constRelAir;
	ConstrelWater constRelWater;

	static const double R = 8.314472; // universal gas constant [J / (mole * K)]
};

/**\ingroup properties
 * @brief property class for liquid phase of air/water mixture
 */
class Liq_WaterAir : public Liquid_GL
{
public:
	virtual double density(double T, double p, double Xa=0.) const // [kg / m^3]
	{
		return constRelWater.mass_density_water_IAPWS(T, p);
	}

	virtual double viscosity(double T, double p, double Xa=0.) const
	{
		return constRelWater.viscosity_water(T,p);
	}

	virtual double intEnergy(double T, double p, double Xa=0.) const
	{
		if (T < 273.15) return 4000 * (T-273.15);
		return constRelWater.enthalpy_water(T,p);
	}

	virtual double enthalpy(double T, double p, double Xa=0.) const
	{
		if (T < 273.15) return 4000 * (T-273.15);
		return constRelWater.enthalpy_water(T,p);
	}

	virtual double diffCoeff(double T, double p) const
	{
		return 2e-9 * T / 273.15;
	}

	virtual double henry(double T) const
	{
		// after Finsterle 1993
		return (0.8942 + 1.47 * exp(-0.04394*T) )*1e-10; // [1/Pa]
	}

	virtual double Xa_Max(double T, double p) const
	{
		FieldVector<double,2> x(henry(T) * p); x[1] = 1- x[0];
		x = this->x2X(x);
		return x[0];
	}

	virtual double p_vap(double T) const
	{
		return constRelAir.pwsat_antoine(T); //[Pa]
	}

	virtual double T_vap(double p) const
	{
		static const double A = 8.19621;
		static const double B = 1730.63;
		static const double C = 233.436;

		p /= 100; // 100 Pa = 1 mbar
		double T = B / (A-log10(p)) - C; // [°C]
		T += 273.15; // [K]
		return T;
	}

  Liq_WaterAir() : Liquid_GL()
	{
  	M_w = 0.018016;
  	M_a = 0.02896;
	}

private:
	ConstrelWater constRelWater;
	ConstrelAir constRelAir;

};

} // namespace

#endif /*PHASEPROPERTIES_HH_*/
