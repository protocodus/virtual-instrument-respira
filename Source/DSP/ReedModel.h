#pragma once

// Free-reed cell of an accordion: one reed chamber with its push reed, its pull
// reed and the pallet hole that vents the chamber to the room.
//
// Air path (push): bellows -> push reed slot -> chamber -> pallet hole -> room.
// Air path (pull): room -> pallet hole -> chamber -> pull reed slot -> bellows.
// Each reed is blown closed (the pressure drop pushes its tongue into its slot)
// and has a leather valve on the far side of the plate, so it passes air in
// one direction only: the push reed speaks on push, the pull reed on pull.
//
// Self-oscillation: the air in the slot and in the pallet hole has inertia.
// As the tongue closes the gap the slot flow cannot stop at once, so the
// pressure drop across the narrowing gap (and the force it puts on the tongue)
// rises while the tongue moves into the slot: the force leads the
// displacement, which pumps energy into the tongue (a blown-closed valve on an
// inertive load; Fletcher 1979, Ricot, Causse & Misdariis 2005, Millot &
// Baumann 2007). The tongue itself moves almost sinusoidally; the sound is the
// pulsed volume flow through the gap, radiated from the pallet hole.
//
// The tongue is its first cantilever mode (tip coordinate). Geometry comes from
// typical accordion reed dimensions scaled with pitch; nothing here is a
// measurement of a particular instrument.

#include "DspMath.h"

#include <cmath>

namespace respira {

constexpr double kPi = 3.14159265358979323846;
constexpr double kRho = 1.2;          // air density, kg/m^3
constexpr double kRhoC2 = 1.4118e5;   // rho * c^2, Pa (c = 343 m/s)
constexpr double kCd = 0.65;          // discharge coefficient of the reed gap
constexpr double kSteelDensity = 7850.0;
constexpr double kSteelE = 2.0e11;
// Laminar term of the gap pressure drop, so the flow law stays smooth near zero
// flow: it equals the Bernoulli term at about 1 m/s.
constexpr double kLaminar = 0.6;

struct ReedSpec {
    double length = 0.026, width = 0.0024;   // tongue, m
    double plate = 0.0012;                   // slot depth (plate thickness), m
    double rest = -0.0004;                   // tip rest position ("set"), m; < 0 is above the plate
    double clearArea = 0.0;                  // gap area while the tongue is inside the slot, m^2
    double area = 0.0;                       // effective pressure area of the tip coordinate, m^2
    double invMass = 0.0, omega2 = 0.0, damping = 0.0; // 1/m_eff, (2 pi f0)^2, omega0 / Q
    double drag = 0.0;                       // quadratic air drag on the tongue / m_eff, 1/m
    double slotInertance = 0.0;              // rho * depth / slot area, kg/m^4
    double breath = 0.0;                     // turbulent fluctuation of the gap flow per unit noise
    double edge = 0.0, cube = 0.0, cube3 = 0.0;   // 2L/3 + W, L^3, 3L^2: GapArea's constants, made once
};

struct ReedState {
    double x = 0.0, v = 0.0;   // tip displacement into the slot (m), velocity (m/s)
    double flow = 0.0;         // volume flow through the gap, m^3/s (>= 0, the valve blocks reverse flow)
};

// Physical reed for a target natural frequency. The tongue is a steel
// cantilever whose length and width follow typical accordion reed sizes; its
// thickness is whatever gives f0 with that length. Real tongues are profiled
// (thinner towards the tip), which leaves less mass at the tip than a uniform
// strip; short treble tongues are profiled hardest. profileMass is that share,
// a voicing estimate (0.6 for long tongues down to 0.3 for 10 mm ones).
inline ReedSpec MakeReed(double f0, double q, double setScale = 1.0) {
    // q is the bare tongue's quality factor in air.
    ReedSpec r;
    r.length = std::fmin(0.075, std::fmax(0.008, 0.026 * std::pow(440.0 / f0, 0.45)));
    r.width = std::fmin(0.0045, std::fmax(0.0016, 0.0024 * std::pow(440.0 / f0, 0.15)));
    r.plate = 0.0012 + 0.015 * r.length;
    r.rest = -(0.0001 + 0.010 * r.length) * setScale;
    const double clearance = 0.03e-3;
    r.clearArea = (2.0 * r.length + r.width) * clearance;
    // First cantilever mode: f = 0.1615 (t / L^2) sqrt(E / rho).
    const double thickness = f0 * r.length * r.length / (0.1615 * std::sqrt(kSteelE / kSteelDensity));
    const double profileMass = 0.3 + 0.3 * std::fmin(1.0, std::fmax(0.0, (r.length - 0.010) / 0.020));
    const double tongueMass = profileMass * 0.25 * kSteelDensity * r.length * r.width * thickness;
    r.area = 0.39 * r.length * r.width;
    const double w0 = 2.0 * kPi * f0;
    // Low reeds carry brass tip weights, which keep the tongue stiff enough not
    // to be blown statically through its slot: at most an eighth of the slot
    // depth of static deflection at 1 kPa. The weight adds mass, not air loss,
    // so the damping stays that of the bare tongue (and Q rises).
    const double stiffness = std::fmax(tongueMass * w0 * w0, r.area * 1000.0 / (0.125 * r.plate));
    const double mass = stiffness / (w0 * w0);
    r.invMass = 1.0 / mass;
    r.omega2 = w0 * w0;
    r.damping = tongueMass * w0 / q / mass;
    // Face-on drag of an oscillating plate, F = rho/2 * Cd * A * v|v|. At these
    // small excursion-to-width ratios Cd is several times the steady value of 2;
    // 8 is a voicing estimate that keeps tip excursions near a tenth of the length.
    r.drag = 0.5 * kRho * 8.0 * r.area / mass;
    r.slotInertance = kRho * r.plate / (r.length * r.width);
    // The jet through the gap is turbulent (a Reynolds number near 10^3 at
    // 26 m/s through a 0.5 mm gap), so its volume flow fluctuates. Eddies the
    // size of the gap, ~0.3 mm, are roughly independent along the 2L + W edge
    // the jet leaves by, each with a turbulence intensity of ~10 %: the total
    // flow fluctuates by 0.1 / sqrt(N) of itself. The noise it is scaled by is
    // uniform, with an RMS of 1 / sqrt(3).
    r.breath = std::sqrt(3.0) * 0.1 / std::sqrt((2.0 * r.length + r.width) / 0.3e-3);
    r.edge = 2.0 * r.length / 3.0 + r.width;
    r.cube = r.length * r.length * r.length;
    r.cube3 = 3.0 * r.length * r.length;
    return r;
}

// Gap area for tip displacement x. The tongue bends as (s/L)^2 along its length,
// so its sides open as a wedge. Inside the slot only the clearance passes air;
// past the slot the part of the tongue below the plate opens again.
inline double GapArea(const ReedSpec& r, double x) {
    double open = 0.0;
    if (x < 0.0) {
        open = r.edge * -x;
    } else if (x > r.plate) {
        const double L = r.length, s = L * Sqrt(r.plate / x);
        open = r.width * (x - r.plate) + 2.0 * (x * (r.cube - s * s * s) / r.cube3 - r.plate * (L - s));
    }
    return Sqrt(open * open + r.clearArea * r.clearArea);
}

// One step of a reed with pressure drop dp (upstream minus downstream).
// The gap flow obeys unsteady Bernoulli, M dU/dt = dp - (rho/2)(U/(Cd A))^2 - lam U/A,
// solved implicitly (the gap can be tiny, which makes it stiff). The tongue
// feels the part of dp that falls across the gap itself. Returns the volume
// leaving the upstream side per second (gap flow plus the volume the tongue
// pushes through the slot). tune2 scales the tongue's stiffness, so its pitch
// moves by sqrt(tune2): the pitch wheel, which no real reed has.
inline double StepReed(const ReedSpec& r, ReedState& s, double dp, double dt, double invDt, double force,
                       double tune2 = 1.0, double flowNoise = 0.0) {
    const double area = GapArea(r, s.x);
    const double inertia = r.slotInertance * invDt;
    const double c = dp + inertia * s.flow;
    double flow = 0.0, drop = 0.0;
    if (c > 0.0) {   // else the valve holds the gap shut: the silent reed of the pair
        const double a = 0.5 * kRho / (kCd * kCd * area * area);
        const double b = kLaminar / area + inertia;
        flow = 2.0 * c / (b + Sqrt(b * b + 4.0 * a * c));
        drop = dp - inertia * (flow - s.flow);
    }
    s.flow = flow;
    s.v += dt * ((r.area * drop + force) * r.invMass - r.omega2 * tune2 * (s.x - r.rest)
                 - (r.damping + r.drag * Abs(s.v)) * s.v);
    s.x += dt * s.v;
    // The tongue only acts as a piston between the two sides while it seals the
    // slot; above or below the plate the air it pushes escapes around it.
    return flow * (1.0 + r.breath * flowNoise) + r.area * s.v * (r.clearArea / area);
}

struct ChamberSpec {
    double stiffness = 0.0;          // rho c^2 / volume, Pa per m^3
    double ventArea = 0.0;           // this chamber's share of the open pallet hole, m^2
    double palletInertance = 0.0;    // air mass of the open pallet hole, kg/m^4
    double channelInertance = 0.0;   // air mass of the chamber channel leading to it, kg/m^4
};

// The reed chamber is a channel alongside the tongue, open at one end to the
// pallet hole. Its depth grows with the reed; a chamber whose Helmholtz
// resonance would fall near the reed is made smaller, as reed makers do with
// wax in the top octaves, because a blown-closed reed only speaks on an
// inertive (below-resonance) load.
//
// As a lumped resonator it holds only up to about that resonance: the channel
// is as long as the reed, so k L is near 1 there. Above it a real chamber's
// standing waves are not modelled, and this one falls 12 dB per octave
// (Tools/CompareHarmonics.cpp, README Known gaps).
//
// Bass-side reeds sit behind the larger pallets and longer air passages of the
// bass mechanism (bassBox): 1.7x the hole area and twice the air path. Without
// that the long bass tongues, choked by a small hole, fall 50+ cents flat
// between soft and loud playing.
inline ChamberSpec MakeChamber(const ReedSpec& r, double f0, bool bassBox = false) {
    ChamberSpec c;
    const double width = r.width + 0.004;
    double depth = 0.003 + 0.2 * r.length;
    const double path = bassBox ? 2.0 : 1.0;
    c.ventArea = 6.0e-5 * std::sqrt(r.length / 0.026) * (bassBox ? 1.7 : 1.0);
    c.palletInertance = path * kRho * (0.006 + 1.7 * std::sqrt(c.ventArea / kPi)) / c.ventArea;
    const auto inertance = [&] { return c.palletInertance + path * kRho * 0.6 * r.length / (width * depth); };
    const double minResonance = 1.4 * f0, w = 2.0 * kPi * minResonance;
    // f_H = 1 / (2 pi sqrt(M V / (rho c^2))): shrink the depth until f_H >= minResonance.
    for (int i = 0; i < 40 && kRhoC2 / (inertance() * r.length * width * depth) < w * w; ++i) depth *= 0.9;
    c.channelInertance = inertance() - c.palletInertance;
    c.stiffness = kRhoC2 / (r.length * width * depth);
    return c;
}

struct CellState {
    ReedState push, pull;
    double pressure = 0.0;   // chamber pressure, Pa (relative to the room)
    double vent = 0.0;       // flow out of the pallet hole, m^3/s
};

// The pallet hole's flow law at opening `open` (0 closed .. 1 open) and time
// step 1 / invDt. It changes only while the pallet moves, so the engine keeps
// it from step to step.
struct Vent {
    double inertia = 0.0, a = 0.0, b = 0.0, b2 = 0.0;
    bool open = false;
};

inline Vent MakeVent(const ChamberSpec& c, double open, double invDt) {
    Vent v;
    if (open <= 0.0) return v;
    const double area = c.ventArea * open;
    v.inertia = (c.channelInertance + c.palletInertance / open) * invDt;
    v.a = 0.5 * kRho / (kCd * kCd * area * area);
    v.b = kLaminar / area + v.inertia;
    v.b2 = v.b * v.b;
    v.open = true;
    return v;
}

// Flow through the pallet hole (positive out to the room), implicit like the gap.
inline double StepVent(const Vent& v, double pressure, double flow) {
    if (!v.open) return 0.0;
    const double drive = pressure + v.inertia * flow;
    const double m = Abs(drive);
    const double out = 2.0 * m / (v.b + Sqrt(v.b2 + 4.0 * v.a * m));
    return drive >= 0.0 ? out : -out;
}

// One step of a whole cell at bellows pressure pb through the pallet hole
// `vent`. Returns the volume flow out of the pallet hole's time derivative (the
// radiated monopole source strength) and adds the flow drawn from the bellows
// to `fromBellows`.
inline double StepCell(const ReedSpec& push, const ReedSpec& pull, const ChamberSpec& chamber, const Vent& hole,
                       CellState& s, double pb, double dt, double invDt, double pushForce, double pullForce,
                       double& fromBellows, double tune2 = 1.0, double flowNoise = 0.0) {
    const double intoChamber = StepReed(push, s.push, pb - s.pressure, dt, invDt, pushForce, tune2, flowNoise);
    const double outOfChamber = StepReed(pull, s.pull, s.pressure - pb, dt, invDt, pullForce, tune2, flowNoise);
    const double vent = StepVent(hole, s.pressure, s.vent);
    const double radiated = (vent - s.vent) * invDt;
    s.vent = vent;
    s.pressure += dt * chamber.stiffness * (intoChamber - outOfChamber - vent);
    fromBellows += intoChamber - outOfChamber;
    return radiated;
}

} // namespace respira
