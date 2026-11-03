# SI Señor

`senor` is a header-only C++20 library for type-safe physical quantities,
units, dimensions, and scales. Dimensions are exponent vectors used as
non-type template parameters, so invalid dimensional operations are rejected
at compile time.

The library is a single public header: `senor.hpp`.

```c++
#include <senor/senor.hpp>
```

Compile with C++20 and put the parent of this repository on the include path.
For example:

```sh
c++ -std=c++20 -Ipath/to -c example.cpp
```

## Quick start

```c++
#include <senor/senor.hpp>

using namespace senor::si;

auto distance = 12.5 * km;       // measure<length>
auto meters = distance.as(m);    // 12500

auto speed = distance / (2.0 * hour);
auto mps = speed.as(meter_per_second);

auto force = 3.0 * (kg * meter_per_second_squared);
auto newtons = force.as(N);
```

Values are stored internally in coherent base units. `as(unit)` converts back
to the requested unit.

## Prefixes

Prefixes are constrained scale objects. They can scale `unit` and
`delta_unit`, but not `affine_unit`:

```c++
using namespace senor;
using namespace senor::si;

auto millimeter = prefix::milli(meter);
auto micrometer = prefix::micro(meter); // same as μm
auto megapascal = prefix::mega(pascal);
```

The full SI prefix set is available, from `quecto` through `quetta`.

## Temperature: points and deltas

Temperature uses two distinct quantity types:

- `Temperature` is a `point<temperature>`: an absolute position on a scale.
- `TemperatureDelta` is a `measure<temperature>`: a difference.

Affine units such as `degC` and `degF` only convert absolute points. Interval
units such as `delta_degC` and `delta_degF` only convert differences.

```c++
auto fever = Temperature::of(37.0, degC);
auto kelvin = fever.as(K);       // 310.15
auto fahrenheit = fever.as(degF);

auto difference = fever - Temperature::of(36.5, degC);
auto delta_k = difference.as(K); // 0.5

auto warmer = fever + TemperatureDelta::of(1.0, K);
auto interval = TemperatureDelta::of(9.0, delta_degF); // 5 K
```

Because `K` is a linear interval unit, `5.0 * K` is a `TemperatureDelta`.
Absolute Kelvin values use `Temperature::of(300.0, K)`.

Invalid operations do not compile:

```c++
// auto bad = fever + Temperature::of(0.0, degC); // no point + point
// auto bad = fever * 2.0;                         // no point scaling
// auto bad = senor::prefix::kilo(degC);           // affine units cannot be prefixed
// auto bad = fever.as(delta_degC);                // point cannot use a delta unit
```

## Custom base dimensions

The SI definitions are parametrized by an enum of base dimensions. A custom
enum can add its own dimensions as long as it provides the SI base names and
an `END` sentinel:

```c++
enum class MyBase {
    length, time, mass, temperature, current,
    luminous_intensity, angle, amount,
    data, END
};

using MySI = senor::si_system<MyBase>;

auto length = 3.0 * MySI::m;
auto rate = 8.0 * MySI::m / (2.0 * MySI::s);
```

The generic engine is also usable without the SI definitions:

```c++
namespace mine {

enum class Base { length, time, END };

using dimension = senor::dimension<Base>;
inline constexpr auto length = dimension::base(Base::length);
inline constexpr auto time = dimension::base(Base::time);

inline constexpr auto meter = senor::unit<length>{1.0L};
inline constexpr auto second = senor::unit<time>{1.0L};
inline constexpr auto meter_per_second = meter / second;

using Length = senor::measure<length>;
using Velocity = senor::measure<meter_per_second.dim>;

} // namespace mine
```

## Core model

- `dimension<E>` — a vector of exponents over the base dimensions in `E`.
- `unit<D>` — a ratio unit such as metre, second, or kelvin.
- `affine_unit<D>` — an absolute affine unit such as Celsius or Fahrenheit.
- `delta_unit<D>` — an interval unit such as a Celsius or Fahrenheit degree.
- `measure<D>` — a vector/delta quantity.
- `point<D>` — an affine-space point such as absolute temperature.

`unit` supports multiplication, division, integer powers, and inversion.
`affine_unit` and `delta_unit` deliberately do not.

## Tests

The test suite is a single source file, `tests/test.cpp`, built with a POSIX
makefile and [depi](https://github.com/josealvim/depi):

```sh
make test
```

`depi` is looked up in `PATH`, with a fallback to the sibling `../depi/depi`
checkout. To use a different binary:

```sh
make DEPI=/path/to/depi test
```

`make clean` removes the test binary and generated dependency file.

## Legal

This is free software, licensed under GPLv3 (or later, of course). If this
license doesn't work for you, contact me. But I forewarn that I tend to prefer
_viral_ FOSS licenses over others.

## Logo

Please don't cancel me or whatever, it's just a pun.

- Chevron-moustache by – Mushon Zer-Aviv, Yanka. Under
  Creative Commons Attribution-Share Alike 3.0 Unported.
  [Source](http://www.wikipediaillustrated.org/meta/cropping-the-moustache).

- Capital Greek letter psi by – GabrielGGD. Under
  Creative Commons CC0 1.0 Universal Public Domain Dedication.
  [Source](https://commons.wikimedia.org/wiki/File:Psi_%28greek_letter%29.svg).
