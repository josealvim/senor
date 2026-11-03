#pragma once

// SPDX-License-Identifier: GPL-3.0-or-later

// senor - type-safe physical quantities, units, and dimensions.
//
// The core model is:
//	 dimension<E>	 an exponent vector over the base dimensions in enum E
//	 unit<D>		 a ratio unit (m, s, K)
//	 affine_unit<D>  an absolute affine unit (degC, degF)
//	 delta_unit<D>	 an interval unit (degree Celsius/Fahrenheit differences)
//	 measure<D>		 a vector/delta quantity
//	 point<D>		 a point/absolute quantity (absolute temperature)
//
// Dimensions are non-type template parameters, so invalid operations are
// rejected at compile time.

#include <array>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <type_traits>

namespace senor {

using real = long double;
using exponent = std::int8_t;

template <typename E>
concept base_enum = std::is_enum_v<E> && requires { E::END; };

template <base_enum E>
struct dimension {
	using enum_type = E;
	static constexpr std::size_t rank = static_cast<std::size_t>(E::END);

	std::array<exponent, rank> exponents{};

	constexpr dimension() = default;

	static constexpr dimension base(E e, exponent n = 1) {
		dimension result;
		result.exponents[static_cast<std::size_t>(e)] = n;
		return result;
	}

	constexpr exponent operator[](E e) const {
		return exponents[static_cast<std::size_t>(e)];
	}

	constexpr bool is_dimensionless() const {
		for (std::size_t i = 0; i < rank; ++i) {
			if (exponents[i] != 0) return false;
		}
		return true;
	}

	constexpr dimension operator+(dimension const& rhs) const {
		dimension result;
		for (std::size_t i = 0; i < rank; ++i) {
			result.exponents[i] = static_cast<exponent>(exponents[i] + rhs.exponents[i]);
		}
		return result;
	}

	constexpr dimension operator-(dimension const& rhs) const {
		dimension result;
		for (std::size_t i = 0; i < rank; ++i) {
			result.exponents[i] = static_cast<exponent>(exponents[i] - rhs.exponents[i]);
		}
		return result;
	}

	constexpr dimension operator-() const {
		dimension result;
		for (std::size_t i = 0; i < rank; ++i) {
			result.exponents[i] = static_cast<exponent>(-exponents[i]);
		}
		return result;
	}

	constexpr dimension operator*(int n) const {
		dimension result;
		for (std::size_t i = 0; i < rank; ++i) {
			result.exponents[i] = static_cast<exponent>(exponents[i] * n);
		}
		return result;
	}

	constexpr bool operator==(dimension const&) const = default;
};

template <typename T>
inline constexpr bool is_dimension_v = false;

template <base_enum E>
inline constexpr bool is_dimension_v<dimension<E>> = true;

template <typename T>
concept dimension_type = is_dimension_v<std::remove_cv_t<T>>;

template <auto D>
concept dimension_value = dimension_type<decltype(D)>;

template <auto D>
using dimension_of = std::remove_cv_t<decltype(D)>;

template <auto D1, auto D2>
concept same_base = dimension_value<D1> && dimension_value<D2> &&
	std::same_as<
		typename dimension_of<D1>::enum_type,
		typename dimension_of<D2>::enum_type>;

template <dimension_type auto D>
class unit {
	public:
	static constexpr auto dim = D;

	constexpr explicit unit(real scale = 1.0L): scale_(scale) {}

	constexpr real scale() const { return scale_; }

	constexpr unit scaled(real factor) const {
		return unit{scale_ * factor};
	}

	template <int N>
	constexpr auto pow() const {
		real factor = 1.0L;
		for (int i = 0; i < (N < 0 ? -N : N); ++i) {
			factor *= scale_;
		}
		if constexpr (N < 0) {
			factor = 1.0L / factor;
		}
		return unit<(D * N)>{factor};
	}

	constexpr auto squared() const { return pow<2>(); }
	constexpr auto cubed() const { return pow<3>(); }
	constexpr auto inverse() const { return unit<(-D)>{1.0L / scale_}; }

	private:
	real scale_;
};

template <dimension_type auto D>
class affine_unit {
	public:
	static constexpr auto dim = D;

	constexpr affine_unit(real scale, real offset): scale_(scale), offset_(offset) {}

	constexpr real scale() const { return scale_; }
	constexpr real offset() const { return offset_; }

	constexpr real to_base(real value) const {
		return value * scale_ + offset_;
	}

	constexpr real from_base(real base) const {
		return (base - offset_) / scale_;
	}

	private:
	real scale_;
	real offset_;
};

template <dimension_type auto D>
class delta_unit {
	public:
	static constexpr auto dim = D;

	constexpr explicit delta_unit(real scale = 1.0L): scale_(scale) {}

	constexpr real scale() const { return scale_; }

	constexpr delta_unit scaled(real factor) const {
		return delta_unit{scale_ * factor};
	}

	private:
	real scale_;
};

template <dimension_type auto D>
class measure {
	public:
	static constexpr auto dim = D;

	constexpr measure() = default;
	constexpr explicit measure(real base): base_(base) {}

	static constexpr measure of(real quantity, unit<D> u) {
		return measure{quantity * u.scale()};
	}

	static constexpr measure of(real quantity, delta_unit<D> u) {
		return measure{quantity * u.scale()};
	}

	constexpr real as(unit<D> u) const {
		return base_ / u.scale();
	}

	constexpr real as(delta_unit<D> u) const {
		return base_ / u.scale();
	}

	constexpr real base_value() const { return base_; }

	constexpr measure abs() const {
		return measure{base_ < 0.0L ? -base_ : base_};
	}

	constexpr measure operator-() const {
		return measure{-base_};
	}

	constexpr measure operator+(measure const& rhs) const {
		return measure{base_ + rhs.base_};
	}

	constexpr measure operator-(measure const& rhs) const {
		return measure{base_ - rhs.base_};
	}

	constexpr measure& operator+=(measure const& rhs) {
		base_ += rhs.base_;
		return *this;
	}

	constexpr measure& operator-=(measure const& rhs) {
		base_ -= rhs.base_;
		return *this;
	}

	constexpr measure operator*(real x) const {
		return measure{base_ * x};
	}

	constexpr measure operator/(real x) const {
		return measure{base_ / x};
	}

	constexpr measure& operator*=(real x) {
		base_ *= x;
		return *this;
	}

	constexpr measure& operator/=(real x) {
		base_ /= x;
		return *this;
	}

	friend constexpr measure operator*(real x, measure const& m) {
		return measure{m.base_ * x};
	}

	constexpr bool operator==(measure const&) const = default;
	constexpr auto operator<=>(measure const&) const = default;

	private:
	real base_{};
};

template <dimension_type auto D>
class point {
	public:
	static constexpr auto dim = D;

	constexpr point() = default;
	constexpr explicit point(real base): base_(base) {}

	static constexpr point of(real quantity, unit<D> u) {
		return point{quantity * u.scale()};
	}

	static constexpr point of(real quantity, affine_unit<D> u) {
		return point{u.to_base(quantity)};
	}

	constexpr real as(unit<D> u) const {
		return base_ / u.scale();
	}

	constexpr real as(affine_unit<D> u) const {
		return u.from_base(base_);
	}

	constexpr real base_value() const { return base_; }

	constexpr measure<D> operator-(point const& rhs) const {
		return measure<D>{base_ - rhs.base_};
	}

	constexpr point operator+(measure<D> delta) const {
		return point{base_ + delta.base_value()};
	}

	constexpr point operator-(measure<D> delta) const {
		return point{base_ - delta.base_value()};
	}

	constexpr point& operator+=(measure<D> delta) {
		base_ += delta.base_value();
		return *this;
	}

	constexpr point& operator-=(measure<D> delta) {
		base_ -= delta.base_value();
		return *this;
	}

	constexpr bool operator==(point const&) const = default;
	constexpr auto operator<=>(point const&) const = default;

	private:
	real base_{};
};

template <dimension_type auto D>
constexpr auto operator+(measure<D> delta, point<D> p) -> point<D> {
	return p + delta;
}

template <dimension_type auto D1, dimension_type auto D2>
	requires same_base<D1, D2>
constexpr auto operator*(unit<D1> lhs, unit<D2> rhs) -> unit<(D1 + D2)> {
	return unit<(D1 + D2)>{lhs.scale() * rhs.scale()};
}

template <dimension_type auto D1, dimension_type auto D2>
	requires same_base<D1, D2>
constexpr auto operator/(unit<D1> lhs, unit<D2> rhs) -> unit<(D1 - D2)> {
	return unit<(D1 - D2)>{lhs.scale() / rhs.scale()};
}

template <dimension_type auto D1, dimension_type auto D2>
	requires same_base<D1, D2>
constexpr auto operator*(measure<D1> lhs, measure<D2> rhs) -> measure<(D1 + D2)> {
	return measure<(D1 + D2)>{lhs.base_value() * rhs.base_value()};
}

template <dimension_type auto D1, dimension_type auto D2>
	requires same_base<D1, D2>
constexpr auto operator/(measure<D1> lhs, measure<D2> rhs) -> measure<(D1 - D2)> {
	return measure<(D1 - D2)>{lhs.base_value() / rhs.base_value()};
}

template <dimension_type auto D>
constexpr auto operator/(real value, measure<D> m) -> measure<(-D)> {
	return measure<(-D)>{value / m.base_value()};
}

template <dimension_type auto D>
constexpr auto operator*(real value, unit<D> u) -> measure<D> {
	return measure<D>::of(value, u);
}

template <dimension_type auto D>
constexpr auto operator*(real value, affine_unit<D> u) -> point<D> {
	return point<D>::of(value, u);
}

template <dimension_type auto D>
constexpr auto operator*(real value, delta_unit<D> u) -> measure<D> {
	return measure<D>::of(value, u);
}

namespace prefix {

struct scale {
	real factor;

	template <dimension_type auto D>
	constexpr unit<D> operator()(unit<D> u) const {
		return u.scaled(factor);
	}

	template <dimension_type auto D>
	constexpr delta_unit<D> operator()(delta_unit<D> u) const {
		return u.scaled(factor);
	}
};

template <dimension_type auto D>
constexpr unit<D> operator*(scale s, unit<D> u) {
	return u.scaled(s.factor);
}

template <dimension_type auto D>
constexpr delta_unit<D> operator*(scale s, delta_unit<D> u) {
	return u.scaled(s.factor);
}

inline constexpr scale quecto{1e-30L};
inline constexpr scale ronto{1e-27L};
inline constexpr scale yocto{1e-24L};
inline constexpr scale zepto{1e-21L};
inline constexpr scale atto{1e-18L};
inline constexpr scale femto{1e-15L};
inline constexpr scale pico{1e-12L};
inline constexpr scale nano{1e-9L};
inline constexpr scale micro{1e-6L};
inline constexpr scale milli{1e-3L};
inline constexpr scale centi{1e-2L};
inline constexpr scale deci{1e-1L};
inline constexpr scale deca{1e1L};
inline constexpr scale deka{1e1L};
inline constexpr scale hecto{1e2L};
inline constexpr scale kilo{1e3L};
inline constexpr scale mega{1e6L};
inline constexpr scale giga{1e9L};
inline constexpr scale tera{1e12L};
inline constexpr scale peta{1e15L};
inline constexpr scale exa{1e18L};
inline constexpr scale zetta{1e21L};
inline constexpr scale yotta{1e24L};
inline constexpr scale ronna{1e27L};
inline constexpr scale quetta{1e30L};

} // namespace prefix

enum class si_base: std::uint8_t {
	length,
	time,
	mass,
	temperature,
	current,
	luminous_intensity,
	angle,
	amount,
	END
};

template <typename E>
concept si_bases = base_enum<E> && requires {
	E::length;
	E::time;
	E::mass;
	E::temperature;
	E::current;
	E::luminous_intensity;
	E::angle;
	E::amount;
};

template <si_bases E>
struct si_system {
	using enum_type = E;
	using dimension = senor::dimension<E>;

	static constexpr auto dimensionless = dimension{};

	static constexpr auto length = dimension::base(E::length);
	static constexpr auto time = dimension::base(E::time);
	static constexpr auto mass = dimension::base(E::mass);
	static constexpr auto temperature = dimension::base(E::temperature);
	static constexpr auto current = dimension::base(E::current);
	static constexpr auto luminous_intensity = dimension::base(E::luminous_intensity);
	static constexpr auto angle = dimension::base(E::angle);
	static constexpr auto amount = dimension::base(E::amount);

	static constexpr auto one = unit<dimensionless>{1.0L};

	static constexpr auto meter = unit<length>{1.0L};
	static constexpr auto second = unit<time>{1.0L};
	static constexpr auto kilogram = unit<mass>{1.0L};
	static constexpr auto kelvin = unit<temperature>{1.0L};
	static constexpr auto ampere = unit<current>{1.0L};
	static constexpr auto candela = unit<luminous_intensity>{1.0L};
	static constexpr auto radian = unit<angle>{1.0L};
	static constexpr auto mole = unit<amount>{1.0L};

	static constexpr auto gram = prefix::milli(kilogram);
	static constexpr auto tonne = prefix::kilo(kilogram);

	static constexpr auto minute = second.scaled(60.0L);
	static constexpr auto hour = second.scaled(60.0L * 60.0L);
	static constexpr auto day = second.scaled(60.0L * 60.0L * 24.0L);

	static constexpr auto square_meter = meter.template pow<2>();
	static constexpr auto cubic_meter = meter.template pow<3>();
	static constexpr auto liter = prefix::milli(cubic_meter);
	static constexpr auto hectare = square_meter.scaled(1e4L);

	static constexpr auto degree = radian.scaled(std::numbers::pi_v<real> / 180.0L);
	static constexpr auto steradian = radian.template pow<2>();

	static constexpr auto hertz = second.inverse();
	static constexpr auto newton = kilogram * meter / second.template pow<2>();
	static constexpr auto pascal = newton / square_meter;
	static constexpr auto joule = newton * meter;
	static constexpr auto watt = joule / second;
	static constexpr auto coulomb = ampere * second;
	static constexpr auto volt = watt / ampere;
	static constexpr auto ohm = volt / ampere;
	static constexpr auto siemens = ampere / volt;
	static constexpr auto farad = coulomb / volt;
	static constexpr auto weber = volt * second;
	static constexpr auto tesla = weber / square_meter;
	static constexpr auto henry = weber / ampere;
	static constexpr auto lumen = candela * steradian;
	static constexpr auto lux = lumen / square_meter;
	static constexpr auto becquerel = hertz;
	static constexpr auto gray = joule / kilogram;
	static constexpr auto sievert = gray;
	static constexpr auto katal = mole / second;

	static constexpr auto meter_per_second = meter / second;
	static constexpr auto meter_per_second_squared = meter / second.template pow<2>();
	static constexpr auto radian_per_second = radian / second;
	static constexpr auto radian_per_second_squared = radian / second.template pow<2>();
	static constexpr auto newton_per_meter = newton / meter;
	static constexpr auto pascal_second = pascal * second;
	static constexpr auto kilogram_per_cubic_meter = kilogram / cubic_meter;

	static constexpr auto percent = one.scaled(1e-2L);
	static constexpr auto ppm = one.scaled(1e-6L);

	static constexpr auto bar = pascal.scaled(1e5L);
	static constexpr auto atmosphere = pascal.scaled(101325.0L);
	static constexpr auto electronvolt = joule.scaled(1.602176634e-19L);
	static constexpr auto calorie = joule.scaled(4.184L);
	static constexpr auto kilowatt_hour = prefix::kilo(watt) * hour;

	static constexpr auto celsius = affine_unit<temperature>{1.0L, 273.15L};
	static constexpr auto fahrenheit = affine_unit<temperature>{
		5.0L / 9.0L,
		459.67L * 5.0L / 9.0L
	};

	static constexpr auto celsius_delta = delta_unit<temperature>{1.0L};
	static constexpr auto fahrenheit_delta = delta_unit<temperature>{5.0L / 9.0L};

	using Length = measure<length>;
	using Time = measure<time>;
	using Mass = measure<mass>;
	using Temperature = point<temperature>;
	using TemperatureDelta = measure<temperature>;
	using Current = measure<current>;
	using LuminousIntensity = measure<luminous_intensity>;
	using Angle = measure<angle>;
	using Amount = measure<amount>;

	using Area = measure<square_meter.dim>;
	using Volume = measure<cubic_meter.dim>;
	using Velocity = measure<meter_per_second.dim>;
	using Speed = Velocity;
	using Acceleration = measure<meter_per_second_squared.dim>;
	using AngularVelocity = measure<radian_per_second.dim>;
	using AngularAcceleration = measure<radian_per_second_squared.dim>;
	using Frequency = measure<hertz.dim>;
	using Force = measure<newton.dim>;
	using Pressure = measure<pascal.dim>;
	using Energy = measure<joule.dim>;
	using Power = measure<watt.dim>;
	using ElectricCharge = measure<coulomb.dim>;
	using Voltage = measure<volt.dim>;
	using ElectricPotential = Voltage;
	using ElectricResistance = measure<ohm.dim>;
	using ElectricConductance = measure<siemens.dim>;
	using Capacitance = measure<farad.dim>;
	using MagneticFlux = measure<weber.dim>;
	using MagneticFluxDensity = measure<tesla.dim>;
	using Inductance = measure<henry.dim>;
	using LuminousFlux = measure<lumen.dim>;
	using Illuminance = measure<lux.dim>;
	using Radioactivity = measure<becquerel.dim>;
	using AbsorbedDose = measure<gray.dim>;
	using EquivalentDose = measure<sievert.dim>;
	using CatalyticActivity = measure<katal.dim>;
	using SurfaceTension = measure<newton_per_meter.dim>;
	using DynamicViscosity = measure<pascal_second.dim>;
	using Density = measure<kilogram_per_cubic_meter.dim>;

	static constexpr auto m = meter;
	static constexpr auto s = second;
	static constexpr auto kg = kilogram;
	static constexpr auto g = gram;
	static constexpr auto K = kelvin;
	static constexpr auto A = ampere;
	static constexpr auto cd = candela;
	static constexpr auto rad = radian;
	static constexpr auto mol = mole;
	static constexpr auto N = newton;
	static constexpr auto Hz = hertz;
	static constexpr auto Pa = pascal;
	static constexpr auto J = joule;
	static constexpr auto W = watt;
	static constexpr auto C = coulomb;
	static constexpr auto V = volt;
	static constexpr auto Ω = ohm;
	static constexpr auto S = siemens;
	static constexpr auto F = farad;
	static constexpr auto Wb = weber;
	static constexpr auto T = tesla;
	static constexpr auto H = henry;
	static constexpr auto lm = lumen;
	static constexpr auto lx = lux;
	static constexpr auto Bq = becquerel;
	static constexpr auto Gy = gray;
	static constexpr auto Sv = sievert;
	static constexpr auto kat = katal;

	static constexpr auto min = minute;
	static constexpr auto h = hour;
	static constexpr auto d = day;
	static constexpr auto deg = degree;
	static constexpr auto L = liter;
	static constexpr auto t = tonne;
	static constexpr auto atm = atmosphere;
	static constexpr auto eV = electronvolt;
	static constexpr auto cal = calorie;
	static constexpr auto ha = hectare;
	static constexpr auto kWh = kilowatt_hour;

	static constexpr auto degC = celsius;
	static constexpr auto degF = fahrenheit;
	static constexpr auto delta_degC = celsius_delta;
	static constexpr auto delta_degF = fahrenheit_delta;

	static constexpr auto km = prefix::kilo(meter);
	static constexpr auto cm = prefix::centi(meter);
	static constexpr auto mm = prefix::milli(meter);
	static constexpr auto μm = prefix::micro(meter);
	static constexpr auto nm = prefix::nano(meter);
	static constexpr auto ms = prefix::milli(second);
	static constexpr auto μs = prefix::micro(second);
	static constexpr auto ns = prefix::nano(second);
	static constexpr auto mg = prefix::milli(gram);
	static constexpr auto μg = prefix::micro(gram);
	static constexpr auto kN = prefix::kilo(newton);
	static constexpr auto kPa = prefix::kilo(pascal);
	static constexpr auto MPa = prefix::mega(pascal);
	static constexpr auto kJ = prefix::kilo(joule);
	static constexpr auto kW = prefix::kilo(watt);
	static constexpr auto mL = prefix::milli(liter);
};

using SI = si_system<si_base>;

namespace si {

using system = SI;
using base = si_base;

inline constexpr auto dimensionless = system::dimensionless;
inline constexpr auto length = system::length;
inline constexpr auto time = system::time;
inline constexpr auto mass = system::mass;
inline constexpr auto temperature = system::temperature;
inline constexpr auto current = system::current;
inline constexpr auto luminous_intensity = system::luminous_intensity;
inline constexpr auto angle = system::angle;
inline constexpr auto amount = system::amount;
inline constexpr auto one = system::one;
inline constexpr auto meter = system::meter;
inline constexpr auto second = system::second;
inline constexpr auto kilogram = system::kilogram;
inline constexpr auto kelvin = system::kelvin;
inline constexpr auto ampere = system::ampere;
inline constexpr auto candela = system::candela;
inline constexpr auto radian = system::radian;
inline constexpr auto mole = system::mole;
inline constexpr auto gram = system::gram;
inline constexpr auto tonne = system::tonne;
inline constexpr auto minute = system::minute;
inline constexpr auto hour = system::hour;
inline constexpr auto day = system::day;
inline constexpr auto square_meter = system::square_meter;
inline constexpr auto cubic_meter = system::cubic_meter;
inline constexpr auto liter = system::liter;
inline constexpr auto hectare = system::hectare;
inline constexpr auto degree = system::degree;
inline constexpr auto steradian = system::steradian;
inline constexpr auto hertz = system::hertz;
inline constexpr auto newton = system::newton;
inline constexpr auto pascal = system::pascal;
inline constexpr auto joule = system::joule;
inline constexpr auto watt = system::watt;
inline constexpr auto coulomb = system::coulomb;
inline constexpr auto volt = system::volt;
inline constexpr auto ohm = system::ohm;
inline constexpr auto siemens = system::siemens;
inline constexpr auto farad = system::farad;
inline constexpr auto weber = system::weber;
inline constexpr auto tesla = system::tesla;
inline constexpr auto henry = system::henry;
inline constexpr auto lumen = system::lumen;
inline constexpr auto lux = system::lux;
inline constexpr auto becquerel = system::becquerel;
inline constexpr auto gray = system::gray;
inline constexpr auto sievert = system::sievert;
inline constexpr auto katal = system::katal;
inline constexpr auto meter_per_second = system::meter_per_second;
inline constexpr auto meter_per_second_squared = system::meter_per_second_squared;
inline constexpr auto radian_per_second = system::radian_per_second;
inline constexpr auto radian_per_second_squared = system::radian_per_second_squared;
inline constexpr auto newton_per_meter = system::newton_per_meter;
inline constexpr auto pascal_second = system::pascal_second;
inline constexpr auto kilogram_per_cubic_meter = system::kilogram_per_cubic_meter;
inline constexpr auto percent = system::percent;
inline constexpr auto ppm = system::ppm;
inline constexpr auto bar = system::bar;
inline constexpr auto atmosphere = system::atmosphere;
inline constexpr auto electronvolt = system::electronvolt;
inline constexpr auto calorie = system::calorie;
inline constexpr auto kilowatt_hour = system::kilowatt_hour;
inline constexpr auto celsius = system::celsius;
inline constexpr auto fahrenheit = system::fahrenheit;
inline constexpr auto celsius_delta = system::celsius_delta;
inline constexpr auto fahrenheit_delta = system::fahrenheit_delta;
using Length = system::Length;
using Time = system::Time;
using Mass = system::Mass;
using Temperature = system::Temperature;
using TemperatureDelta = system::TemperatureDelta;
using Current = system::Current;
using LuminousIntensity = system::LuminousIntensity;
using Angle = system::Angle;
using Amount = system::Amount;
using Area = system::Area;
using Volume = system::Volume;
using Velocity = system::Velocity;
using Speed = system::Speed;
using Acceleration = system::Acceleration;
using AngularVelocity = system::AngularVelocity;
using AngularAcceleration = system::AngularAcceleration;
using Frequency = system::Frequency;
using Force = system::Force;
using Pressure = system::Pressure;
using Energy = system::Energy;
using Power = system::Power;
using ElectricCharge = system::ElectricCharge;
using Voltage = system::Voltage;
using ElectricPotential = system::ElectricPotential;
using ElectricResistance = system::ElectricResistance;
using ElectricConductance = system::ElectricConductance;
using Capacitance = system::Capacitance;
using MagneticFlux = system::MagneticFlux;
using MagneticFluxDensity = system::MagneticFluxDensity;
using Inductance = system::Inductance;
using LuminousFlux = system::LuminousFlux;
using Illuminance = system::Illuminance;
using Radioactivity = system::Radioactivity;
using AbsorbedDose = system::AbsorbedDose;
using EquivalentDose = system::EquivalentDose;
using CatalyticActivity = system::CatalyticActivity;
using SurfaceTension = system::SurfaceTension;
using DynamicViscosity = system::DynamicViscosity;
using Density = system::Density;
inline constexpr auto m = system::m;
inline constexpr auto s = system::s;
inline constexpr auto kg = system::kg;
inline constexpr auto g = system::g;
inline constexpr auto K = system::K;
inline constexpr auto A = system::A;
inline constexpr auto cd = system::cd;
inline constexpr auto rad = system::rad;
inline constexpr auto mol = system::mol;
inline constexpr auto N = system::N;
inline constexpr auto Hz = system::Hz;
inline constexpr auto Pa = system::Pa;
inline constexpr auto J = system::J;
inline constexpr auto W = system::W;
inline constexpr auto C = system::C;
inline constexpr auto V = system::V;
inline constexpr auto Ω = system::Ω;
inline constexpr auto S = system::S;
inline constexpr auto F = system::F;
inline constexpr auto Wb = system::Wb;
inline constexpr auto T = system::T;
inline constexpr auto H = system::H;
inline constexpr auto lm = system::lm;
inline constexpr auto lx = system::lx;
inline constexpr auto Bq = system::Bq;
inline constexpr auto Gy = system::Gy;
inline constexpr auto Sv = system::Sv;
inline constexpr auto kat = system::kat;
inline constexpr auto min = system::min;
inline constexpr auto h = system::h;
inline constexpr auto d = system::d;
inline constexpr auto deg = system::deg;
inline constexpr auto L = system::L;
inline constexpr auto t = system::t;
inline constexpr auto atm = system::atm;
inline constexpr auto eV = system::eV;
inline constexpr auto cal = system::cal;
inline constexpr auto ha = system::ha;
inline constexpr auto kWh = system::kWh;
inline constexpr auto degC = system::degC;
inline constexpr auto degF = system::degF;
inline constexpr auto delta_degC = system::delta_degC;
inline constexpr auto delta_degF = system::delta_degF;
inline constexpr auto km = system::km;
inline constexpr auto cm = system::cm;
inline constexpr auto mm = system::mm;
inline constexpr auto μm = system::μm;
inline constexpr auto nm = system::nm;
inline constexpr auto ms = system::ms;
inline constexpr auto μs = system::μs;
inline constexpr auto ns = system::ns;
inline constexpr auto mg = system::mg;
inline constexpr auto μg = system::μg;
inline constexpr auto kN = system::kN;
inline constexpr auto kPa = system::kPa;
inline constexpr auto MPa = system::MPa;
inline constexpr auto kJ = system::kJ;
inline constexpr auto kW = system::kW;
inline constexpr auto mL = system::mL;

} // namespace si

} // namespace senor
