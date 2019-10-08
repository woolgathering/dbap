// PluginDBAP.cpp
// Jacob Sundstrom (jacob.sundstrom@gmail.com)

#include "SC_PlugIn.hpp"
#include "DBAP.hpp"

static InterfaceTable* ft;

namespace DBAP {

DBAP::DBAP() {
    mCalcFunc = make_calc_function<DBAP, &DBAP::next>();
    next(1);
}

void DBAP::next(int nSamples) {
    const float* input = in(0);
    const float* gain = in(0);
    float* outbuf = out(0);

    // simple gain function
    for (int i = 0; i < nSamples; ++i) {
        outbuf[i] = input[i] * gain[i];
    }
}

} // namespace DBAP

PluginLoad(DBAPUGens) {
    // Plugin magic
    ft = inTable;
    registerUnit<DBAP::DBAP>(ft, "DBAP");
}
