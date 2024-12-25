#pragma once
#pragma warning(disable : 4251)

namespace gaia {

class iService {
public:
    virtual bool init() = 0;
    virtual void unit() = 0;

    virtual bool isValid() const = 0;
    virtual bool isThereAnError() const = 0;
};

}  // namespace gaia
