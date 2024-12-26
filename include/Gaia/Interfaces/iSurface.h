#pragma once
#pragma warning(disable : 4251)

template <typename T>
class iSurface {
protected:
    T m_size = T(0);

public:
    virtual void setSize(const T& vSize) = 0;
    virtual const T& getSize() const = 0;
    virtual bool resize(const T& vNewSize) = 0;
};
