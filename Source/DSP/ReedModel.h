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
constexpr double kMaxPressure = 2500.0;  // what the player's arm can put on the bellows, Pa
// Beside the tongue in its slot: a repairer gives 0.0015 in (0.038 mm) beside
// a properly voiced high A's tongue (Accordion Revival).
constexpr double kClearance = 0.03e-3;
// Laminar term of the gap pressure drop, so the flow law stays smooth near zero
// flow: it equals the Bernoulli term at about 1 m/s.
constexpr double kLaminar = 0.6;
constexpr double kMu = 1.8e-5;   // air's viscosity, Pa s
// How far a pallet's pad covers the board round its hole: an estimate, as no
// measurement was found. 1-6 mm moves a half-pressed A#2-A3 within a cent.
constexpr double kPadOverlap = 0.003;

struct ReedSpec {
    double length = 0.026, width = 0.0024;   // tongue, m
    double plate = 0.0012;                   // slot depth (plate thickness), m
    double rest = -0.0004;                   // tip rest position ("set"), m; < 0 is above the plate
    double clearArea = 0.0;                  // gap area while the tongue is inside the slot, m^2
    double area = 0.0;                       // effective pressure area of the tip coordinate, m^2
    double invMass = 0.0, omega2 = 0.0, damping = 0.0; // 1/m_eff, (2 pi f0)^2, omega0 / Q
    double drag = 0.0;                       // quadratic air drag on the tongue / m_eff, 1/m
    double slotInertance = 0.0;              // rho * depth / slot area, kg/m^4
    double breath = 0.0;                     // the gap flow's turbulent fluctuation per unit noise and sqrt(m) of the tip's opening
    double eddyShare = 0.0;                  // the gap's eddies' mean size over the tip's opening
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
// strip; short treble tongues are profiled hardest. profileMass is that share:
// 0.6 for long tongues, falling from 30 mm (E4) to 0.18 at 19 mm (A5) and
// above. 0.18 is a linear taper to the 0.05 mm tip a repairer gives the
// smallest piccolo tongues (Accordion Revival, "Accordion Repair 3") and a
// reed maker the highest notes (rec.music.makers.squeezebox, 2004). Where the
// fall ends, A5, puts A5-B6's attacks at 36-127 ms from -50 to -5 dB, where
// Llanos-Vazquez et al. (2014, Acta Acustica 100, Table I) measured 50-100;
// with 0.3 they took 111-311. The share above 30 mm stays a voicing estimate.
inline ReedSpec MakeReed(double f0, double q, double setScale = 1.0) {
    // q is the bare tongue's quality factor in air.
    ReedSpec r;
    r.length = std::fmin(0.075, std::fmax(0.008, 0.026 * std::pow(440.0 / f0, 0.45)));
    r.width = std::fmin(0.0045, std::fmax(0.0016, 0.0024 * std::pow(440.0 / f0, 0.15)));
    // The reed plate's thickness, the slot's depth: it varies between
    // instruments. 2.0-3.5 mm was chosen by ear over two thirds of it
    // (Docs/decisions.md). At 70 dBA it keeps A#2-A3 inside their slots,
    // which puts their 4th harmonic on the one published measurement
    // (Elejalde-Garcia et al. 2021; Tools/CompareHarmonics). The one plate
    // measured, under a 27 mm B4 tongue, is 3.1 mm (Babanis, NTUA diploma
    // thesis, 2014, Fig. 8.5; caliper), where this gives 2.4; scaled to it,
    // A#2-A3's 2nd harmonic fell 12 dB under that measurement and a
    // half-pressed key fell silent.
    r.plate = 0.0018 + 0.0225 * r.length;
    r.rest = -(0.0001 + 0.010 * r.length) * setScale;
    r.clearArea = (2.0 * r.length + r.width) * kClearance;
    // First cantilever mode: f = 0.1615 (t / L^2) sqrt(E / rho).
    const double thickness = f0 * r.length * r.length / (0.1615 * std::sqrt(kSteelE / kSteelDensity));
    const double profileMass = 0.18 + 0.42 * std::fmin(1.0, std::fmax(0.0, (r.length - 0.019) / 0.011));
    const double tongueMass = profileMass * 0.25 * kSteelDensity * r.length * r.width * thickness;
    r.area = 0.39 * r.length * r.width;
    const double w0 = 2.0 * kPi * f0;
    // Low reeds carry brass tip weights, which keep the tongue stiff enough not
    // to be blown statically through its slot: at most an eighth of the slot
    // depth of static deflection at 1 kPa. A reed the bellows reaches the
    // arm's full pressure on before it swings must still stop short of the
    // slot by its clearance, or it sits at the slot's mouth and never speaks,
    // as an accordion's reeds choke when set too low. Weighting the tongue for
    // that made the low reeds rise more slowly; setting it higher instead was
    // chosen by ear (Docs/decisions.md). At C4 that gives 0.32 mm/kPa of tip
    // compliance and 2.3 Hz/kPa of pitch fall with the pallet open, where Okada
    // measured 0.24 and 2.5 on one reed set 0.3 mm high (this one 0.8 mm;
    // Adachi et al., Forum Acusticum 2026). The weight adds mass, not air
    // loss, so the damping stays that of the bare tongue (and Q rises).
    const double stiffness = std::fmax(tongueMass * w0 * w0, r.area * 1000.0 / (0.125 * r.plate));
    r.rest = std::fmin(r.rest, -(r.area * kMaxPressure / stiffness + kClearance));
    const double mass = stiffness / (w0 * w0);
    r.invMass = 1.0 / mass;
    r.omega2 = w0 * w0;
    r.damping = tongueMass * w0 / q / mass;
    // Face-on drag of an oscillating plate, F = rho/2 * Cd * A * v|v|. At these
    // small excursion-to-width ratios Cd is several times the steady value of 2:
    // a flat plate's measured Cd = 10 KC^-1/3, KC = 2 pi (tip swing) / width
    // (Keulegan and Carpenter 1958, as Graham 1980 fitted it), is 5-10 over the
    // swings of ordinary playing, 0.4-4 mm on tongues 2.2-2.6 mm wide. 8 keeps
    // tip excursions near a tenth of the length.
    r.drag = 0.5 * kRho * 8.0 * r.area / mass;
    r.slotInertance = kRho * r.plate / (r.length * r.width);
    // The jet through the gap is turbulent (a Reynolds number near 10^3 at
    // 26 m/s through a 0.5 mm gap), so its volume flow fluctuates. Its eddies
    // are the size of the gap where they form, h, so along the edge the jet
    // leaves by there are 1/h of them per metre, each passing U h^2 with a
    // turbulence intensity estimated at ~10 % and, by ear, half that
    // (Docs/decisions.md). The gap is the tongue's bend, h = tip (s/L)^2,
    // along both sides and the tip across its width, so the flow fluctuates by
    // 0.05 sqrt(integral h^3) / integral h = 0.05 sqrt(tip (W + 2L/7)) /
    // (W + 2L/3) of itself: most of it from the widest part of the gap, near
    // the tip, whose eddies average (W + 2L/9) / (W + 2L/7) of the tip's
    // opening (the engine shapes the noise to them). A single 0.3 mm eddy size
    // left the low and middle reeds' breath 10-25 dB under a bandoneon's below
    // 2 kHz, against their harmonics. The noise it is scaled by is uniform,
    // with an RMS of 1 / sqrt(3).
    r.breath = std::sqrt(3.0) * 0.05 * std::sqrt(r.width + 2.0 * r.length / 7.0) / (r.width + 2.0 * r.length / 3.0);
    r.eddyShare = (r.width + 2.0 * r.length / 9.0) / (r.width + 2.0 * r.length / 7.0);
    r.edge = 2.0 * r.length / 3.0 + r.width;
    r.cube = r.length * r.length * r.length;
    r.cube3 = 3.0 * r.length * r.length;
    return r;
}

// The same tongue retuned by `scale` at its root, then set as MakeReed sets
// it: high enough that the arm's full pressure cannot blow it into its slot,
// where a softened low reed choked (HardPushSpeaks at -100 cents). Rebuilding
// the reed at the new pitch instead changed its tip share, which left A4 1 cent
// short of a 100-cent master tune.
inline void Retune(ReedSpec& r, double scale) {
    r.omega2 *= scale * scale;
    r.rest = std::fmin(r.rest, -(r.area * kMaxPressure * r.invMass / r.omega2 + kClearance));
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
// moves by sqrt(tune2): the pitch wheel, which no real reed has. flowNoise is
// the jet's turbulent fluctuation as a share of its flow (breath).
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
    return flow * (1.0 + flowNoise) + r.area * s.v * (r.clearArea / area);
}

// The reed chamber as a duct in sections of at most kSectionLength: four or
// more to a wavelength at 16 kHz. Half that length moves the harmonics by under
// 1 dB below 8 kHz and 2.5 dB above (Tools/CompareHarmonics, the demos). The
// bass side once took 10 mm while its bass box took off what they change;
// since the box passes its depth modes (RespiraEngine.cpp), 10 mm moves the
// demos' 8-16 kHz octave by 0.7-6.3 dB against 5 mm, to save under 2 % of
// their work.
constexpr double kSectionLength = 0.005;
constexpr int kMaxSections = 16;   // the longest chamber, 75 mm, in 5 mm sections

struct ChamberSpec {
    double ventArea = 0.0;           // this chamber's share of the open pallet hole, m^2
    double palletInertance = 0.0;    // air mass of the open pallet hole, kg/m^4
    double endInertance = 0.0;       // the part of it that is the outer end's air mass, kg/m^4
    double padFilm = 0.0;            // the air film under the pad's rim, Pa s/m^3 at an opening of 1
    double channelInertance = 0.0;   // air mass from the last section's middle to the hole, kg/m^4
    int sections = 1;
    double sectionStiffness = 0.0;   // rho c^2 / a section's volume, Pa per m^3
    double sectionInertance = 0.0;   // air mass between two sections' middles, kg/m^4
    double weight[kMaxSections] = {};   // each section's share of the gap's flow (and of its pressure on the tongue)
};

// The reed chamber is a channel alongside the tongue, closed at the tongue's
// tip and open at its root end to the pallet hole. Its depth grows with the
// reed; a chamber whose resonance would fall near the reed is made smaller,
// as reed makers do with wax in the top octaves, because a blown-closed reed
// only speaks on an inertive (below-resonance) load.
//
// The channel is as long as the reed, so k L is near 1 at that resonance: as
// a single lumped resonator it held only up to there, and fell 12 dB an octave
// above it, leaving A#2-A3's 15th-20th harmonics 7 dB under the one published
// measurement (Elejalde-Garcia et al. 2021). As a duct it passes its standing
// waves, which puts them on it (Tools/CompareHarmonics). The gap's air enters
// where the gap opens: along both sides as the tongue's (s/L)^2 bend from the
// root, and across the tip (GapArea), and the tongue feels the pressure there
// with the same weights. Tips at the closed end match the measurement; at the
// pallet hole the 15th-20th fell 16 dB under it, and with the two tongues
// riveted at opposite ends some keys stayed silent on a sudden hard push
// (Docs/decisions.md). The duct's wall losses are left out: about 0.3 dB
// over a 61 mm channel at 8 kHz, though at a chamber's first resonance, where
// the air passes many times, they would take another 2-4 dB off its peak.
//
// Bass-side reeds sit behind the larger pallets and longer air passages of the
// bass mechanism (bassBox): 1.7x the hole area and twice the air path. Without
// that the long bass tongues, choked by a small hole, fall 50+ cents flat
// between soft and loud playing.
//
// stepRate is the lowest rate the cell will be stepped at: sound must not
// cross a section in less than a step, or the duct goes unstable.
inline ChamberSpec MakeChamber(const ReedSpec& r, double f0, bool bassBox, double stepRate) {
    ChamberSpec c;
    const double width = r.width + 0.004, L = r.length;
    double depth = 0.003 + 0.2 * L;
    const double path = bassBox ? 2.0 : 1.0;
    c.ventArea = 6.0e-5 * std::sqrt(L / 0.026) * (bassBox ? 1.7 : 1.0);
    c.palletInertance = path * kRho * (0.006 + 1.7 * std::sqrt(c.ventArea / kPi)) / c.ventArea;
    c.endInertance = kRho * 0.85 * std::sqrt(c.ventArea / kPi) / c.ventArea;
    const double rim = 2.0 * std::sqrt(kPi * c.ventArea);
    c.padFilm = 12.0 * kMu * kPadOverlap * rim * rim / (c.ventArea * c.ventArea * c.ventArea);
    const auto inertance = [&] { return c.palletInertance + path * kRho * 0.6 * L / (width * depth); };
    const double minResonance = 1.4 * f0, w = 2.0 * kPi * minResonance;
    // f_H = 1 / (2 pi sqrt(M V / (rho c^2))), lumped: shrink the depth until
    // f_H >= minResonance. The duct's first resonance sits 10-15 % above it.
    for (int i = 0; i < 40 && kRhoC2 / (inertance() * L * width * depth) < w * w; ++i) depth *= 0.9;
    const double area = width * depth;
    const int n = static_cast<int>(Clamp(std::floor(L / Max(kSectionLength, 343.0 / stepRate)), 1.0, kMaxSections));
    const double dx = L / n;
    c.sections = n;
    c.sectionStiffness = kRhoC2 / (area * dx);
    c.sectionInertance = kRho * dx / area;
    // Half a section to the hole; the bass air path's second length as before.
    c.channelInertance = kRho * 0.5 * dx / area + (path - 1.0) * kRho * 0.6 * L / area;
    double sum = 0.0;
    for (int i = 0; i < n; ++i) {   // section 0 at the closed end, under the tip
        const double near = 1.0 - (i + 1) * dx / L, far = 1.0 - i * dx / L;   // distance from the root over L
        c.weight[i] = 2.0 * L / 3.0 * (far * far * far - near * near * near) + (i == 0 ? r.width : 0.0);
        sum += c.weight[i];
    }
    for (int i = 0; i < n; ++i) c.weight[i] /= sum;
    return c;
}

struct CellState {
    ReedState push, pull;
    double vent = 0.0;       // flow out of the pallet hole, m^3/s
    double endFlow = 0.0;    // the part of it that moves the outer end's air mass, m^3/s
    double p[kMaxSections] = {};       // each section's pressure, Pa (relative to the room)
    double u[kMaxSections + 1] = {};   // flow into each section from the closed end's side; u[sections] is the vent
};

// The pallet hole's flow law at opening `open` (0 closed .. 1 open) and time
// step 1 / invDt. It changes only while the pallet moves, so the engine keeps
// it from step to step.
//
// The hole's outer end radiates into the room: its end correction's air mass
// (0.85 a, flanged) works in parallel with the radiation resistance rho c / A,
// Beranek's circuit for a piston in a baffle (Acoustics, 1954): a resistance
// rising as f^2 under the mass well below ka = 1, and rho c / A above. Without
// it the chambers' standing waves rang at Q 70-370 (A3, E4 and A5 under a
// swept flow); with it at 26-48.
//
// A pad lifted h opens a ring round the hole's rim (as a circle, like its end
// corrections), rim h = open A, and the air leaves through the film between
// the pad and the board: plane Poiseuille flow, 12 mu kPadOverlap / (rim h^3).
// It takes over from the orifice below about a tenth of the opening, so the
// flow comes to rest smoothly as the pad lands. Without it the hole's air
// mass, over the closing opening, kept the flow going until the pallet shut
// and cut it in one step: a click at every key release.
struct Vent {
    double inertia = 0.0, a = 0.0, b = 0.0, b2 = 0.0;
    double end = 0.0, endStep = 0.0;   // the outer end's load per step, and dt over its air mass
    bool open = false;
};

inline Vent MakeVent(const ChamberSpec& c, double open, double invDt) {
    Vent v;
    if (open <= 0.0) return v;
    const double area = c.ventArea * open;
    v.inertia = (c.channelInertance + (c.palletInertance - c.endInertance) / open) * invDt;
    v.endStep = open / (c.endInertance * invDt);
    v.end = 1.0 / (v.endStep + area / (kRhoC2 / 343.0));
    v.a = 0.5 * kRho / (kCd * kCd * area * area);
    v.b = kLaminar / area + c.padFilm / (open * open * open) + v.inertia + v.end;
    v.b2 = v.b * v.b;
    v.open = true;
    return v;
}

// Flow through the pallet hole (positive out to the room), implicit like the gap.
inline double StepVent(const Vent& v, double pressure, double flow, double& endFlow) {
    if (!v.open) return endFlow = 0.0;
    const double drive = pressure + v.inertia * flow + v.end * endFlow;
    const double m = Abs(drive);
    const double out = 2.0 * m / (v.b + Sqrt(v.b2 + 4.0 * v.a * m));
    const double flowOut = drive >= 0.0 ? out : -out;
    endFlow += v.endStep * v.end * (flowOut - endFlow);
    return flowOut;
}

// One step of a whole cell at bellows pressure pb through the pallet hole
// `vent`. Returns the volume flow out of the pallet hole's time derivative (the
// radiated monopole source strength) and adds the flow drawn from the bellows
// to `fromBellows`.
inline double StepCell(const ReedSpec& push, const ReedSpec& pull, const ChamberSpec& chamber, const Vent& hole,
                       CellState& s, double pb, double dt, double invDt, double pushForce, double pullForce,
                       double& fromBellows, double tune2 = 1.0, double flowNoise = 0.0) {
    const int n = chamber.sections;
    double reedPressure = 0.0;
    for (int i = 0; i < n; ++i) reedPressure += chamber.weight[i] * s.p[i];
    const double intoChamber = StepReed(push, s.push, pb - reedPressure, dt, invDt, pushForce, tune2, flowNoise);
    const double outOfChamber = StepReed(pull, s.pull, reedPressure - pb, dt, invDt, pullForce, tune2, flowNoise);
    const double vent = StepVent(hole, s.p[n - 1], s.vent, s.endFlow);
    const double radiated = (vent - s.vent) * invDt;
    s.vent = vent;
    // Flows from the old pressures, then pressures from the new flows.
    const double g = dt / chamber.sectionInertance, k = dt * chamber.sectionStiffness, q = k * (intoChamber - outOfChamber);
    for (int i = 1; i < n; ++i) s.u[i] += g * (s.p[i - 1] - s.p[i]);
    s.u[n] = vent;
    for (int i = 0; i < n; ++i) s.p[i] += q * chamber.weight[i] + k * (s.u[i] - s.u[i + 1]);
    fromBellows += intoChamber - outOfChamber;
    return radiated;
}

} // namespace respira
