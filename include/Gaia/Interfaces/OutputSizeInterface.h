#pragma once
#pragma warning(disable : 4251)

#include <ezlibs/ezTools.hpp>
#include <Gaia/gaia.h>

class GAIA_API OutputSizeInterface {
public:
    virtual float GetOutputRatio() const = 0;
    virtual ez::fvec2 GetOutputSize() const = 0;
};
