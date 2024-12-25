#pragma once
#pragma warning(disable : 4251)

#include <Gaia/gaia.h>

namespace gaia {

class GAIA_API iService {
public:
    virtual bool init() = 0;
    virtual void unit() = 0;

    virtual bool isValid() const = 0;
    virtual bool isThereAnError() const = 0;
};

}  // namespace gaia
