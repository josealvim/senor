// SPDX-License-Identifier: GPL-3.0-or-later

#include <senor/senor.hpp>

#include <cassert>
#include <concepts>
#include <numbers>
#include <type_traits>

namespace {

using namespace senor;
using namespace senor::si;

constexpr bool near(real a, real b, real eps = 1e-12L) {
    real diff = a - b;
    if (diff < 0.0L) diff = -diff;

    real magnitude = b < 0.0L ? -b : b;
    if (magnitude < 1.0L) magnitude = 1.0L;

    return diff <= eps * magnitude;
}

} // namespace

// Dimensions are compile-time values.
static_assert(length + time == senor::dimension<si_base>::base(si_base::length, 1)
                                 + senor::dimension<si_base>::base(si_base::time, 1));
static_assert((meter / second).dim == length - time);
static_assert((meter * meter).dim == length * 2);
static_assert((meter * meter).dim == length + length);
static_assert((meter / meter).dim.is_dimensionless());

// Unit algebra.
static_assert(std::same_as<decltype(m / s), unit<(m / s).dim>>);
static_assert(std::same_as<decltype(m * m), unit<(m * m).dim>>);
static_assert(std::same_as<decltype(kilogram * meter / s.pow<2>()), unit<newton.dim>>);

// Measure and point types.
static_assert(std::same_as<decltype(1.0L * m), Length>);
static_assert(std::same_as<decltype(1.0L * km), Length>);
static_assert(std::same_as<decltype(1.0L * degC), Temperature>);
static_assert(std::same_as<decltype(1.0L * delta_degF), TemperatureDelta>);
static_assert(std::same_as<decltype(Temperature::of(0.0L, degC)), Temperature>);
static_assert(std::same_as<decltype(Temperature::of(0.0L, K)), Temperature>);
static_assert(std::same_as<
    decltype(Temperature::of(20.0L, degC) - Temperature::of(10.0L, degC)),
    TemperatureDelta>);
static_assert(std::same_as<
    decltype(TemperatureDelta::of(5.0L, K) + Temperature::of(20.0L, degC)),
    Temperature>);
static_assert(std::same_as<
    std::remove_cv_t<decltype(km)>,
    decltype(prefix::kilo * m)>);
static_assert(std::same_as<
    decltype(prefix::milli(delta_degC)),
    delta_unit<temperature>>);

// Length.
static_assert(near(Length::of(1.0L, km).as(m), 1000.0L));
static_assert(near(Length::of(1.0L, cm).as(m), 0.01L));
static_assert(near(Length::of(1.0L, mm).as(m), 0.001L));
static_assert(near(Length::of(1.0L, μm).as(m), 1e-6L));
static_assert(near(Length::of(1.0L, nm).as(m), 1e-9L));
static_assert(near((1.0L * km).as(m), 1000.0L));
static_assert(near((3.0L * m).as(m), 3.0L));

// Time and speed.
static_assert(near((1.0L * hour).as(s), 3600.0L));
static_assert(near((1.0L * minute).as(s), 60.0L));
static_assert(near((1.0L * day).as(s), 86400.0L));
static_assert(near((1.0L * ms).as(s), 1e-3L));
static_assert(near((1.0L * μs).as(s), 1e-6L));
static_assert(near(
    (1.0L * km / (1.0L * hour)).as(meter_per_second),
    1000.0L / 3600.0L));

// Mass.
static_assert(near((1.0L * mg).as(kg), 1e-6L));
static_assert(near((1.0L * μg).as(kg), 1e-9L));
static_assert(near((1.0L * g).as(kg), 1e-3L));

// Angle.
static_assert(near((1.0L * degree).as(rad), std::numbers::pi_v<real> / 180.0L));
static_assert(near((180.0L * degree).as(rad), std::numbers::pi_v<real>));

// Area and volume.
static_assert(near((1.0L * liter).as(cubic_meter), 1e-3L));
static_assert(near((1.0L * hectare).as(square_meter), 1e4L));

// Derived units.
static_assert(near((1.0L * N).as(kg * meter / s.pow<2>()), 1.0L));
static_assert(near((1.0L * Pa).as(N / square_meter), 1.0L));
static_assert(near((1.0L * J).as(N * m), 1.0L));
static_assert(near((1.0L * W).as(J / s), 1.0L));
static_assert(near((1.0L * V).as(W / A), 1.0L));
static_assert(near((1.0L * Ω).as(V / A), 1.0L));
static_assert(near((1.0L * MPa).as(Pa), 1e6L));
static_assert(near((1.0L * kWh).as(J), 3.6e6L));

// Temperature affine conversions.
static_assert(near(Temperature::of(0.0L, degC).as(K), 273.15L));
static_assert(near(Temperature::of(100.0L, degC).as(degF), 212.0L));
static_assert(near(Temperature::of(32.0L, degF).as(degC), 0.0L));
static_assert(near(Temperature::of(-40.0L, degC).as(degF), -40.0L));
static_assert(near(Temperature::of(0.0L, K).as(degC), -273.15L));

// Temperature point/delta arithmetic.
static_assert(near(
    (Temperature::of(20.0L, degC) - Temperature::of(10.0L, degC)).as(K),
    10.0L));
static_assert(near(
    (Temperature::of(20.0L, degC) + TemperatureDelta::of(5.0L, K)).as(degC),
    25.0L));
static_assert(near(
    (Temperature::of(20.0L, degC) - TemperatureDelta::of(5.0L, K)).as(degC),
    15.0L));
static_assert(near(TemperatureDelta::of(18.0L, delta_degF).as(K), 10.0L));
static_assert(near(TemperatureDelta::of(10.0L, delta_degC).as(K), 10.0L));

// Dimensionless ratios.
static_assert(near(((1.0L * km) / (1.0L * m)).as(one), 1000.0L));

int main() {
    auto length = 3.0L * m;
    length += 2.0L * m;
    assert(near(length.as(m), 5.0L));

    length *= 2.0L;
    assert(near(length.as(m), 10.0L));

    length /= 5.0L;
    assert(near(length.as(m), 2.0L));

    auto negative = -length;
    assert(near(negative.as(m), -2.0L));
    assert(near(negative.abs().as(m), 2.0L));

    auto ratio = (2.0L * m) / (1.0L * m);
    assert(near(ratio.as(one), 2.0L));

    auto reciprocal = 1.0L / (2.0L * s);
    assert(near(reciprocal.as(hertz), 0.5L));

    auto speed = (100.0L * km) / (2.0L * hour);
    assert(near(speed.as(meter_per_second), 1000.0L / 72.0L));

    auto temperature = Temperature::of(20.0L, degC);
    temperature += TemperatureDelta::of(5.0L, K);
    assert(near(temperature.as(degC), 25.0L));

    temperature -= TemperatureDelta::of(10.0L, K);
    assert(near(temperature.as(degC), 15.0L));

    auto shifted = TemperatureDelta::of(5.0L, K) + Temperature::of(20.0L, degC);
    assert(near(shifted.as(degC), 25.0L));

    auto difference = temperature - Temperature::of(0.0L, degC);
    assert(near(difference.as(K), 15.0L));

    auto fahrenheit = Temperature::of(98.6L, degF);
    assert(near(fahrenheit.as(degC), 37.0L));

    assert(Temperature::of(20.0L, degC) > Temperature::of(10.0L, degC));
    assert(TemperatureDelta::of(2.0L, K) > TemperatureDelta::of(1.0L, K));

    return 0;
}
