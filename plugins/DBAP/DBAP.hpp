// PluginDBAP.hpp
// Jacob Sundstrom (jacob.sundstrom@gmail.com)

#pragma once

#include "SC_PlugIn.hpp"

namespace DBAP {

class DBAP : public SCUnit {
public:
    DBAP();

    // Destructor
    // ~DBAP();

private:
    // Calc function
    void next(int nSamples);

    // Member variables
};

} // namespace DBAP
