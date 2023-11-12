#pragma once
#pragma warning(disable : 4251)

#include <ctools/cTools.h>
#include <Gaia/gaia.h>

class GAIA_API OutputSizeInterface {
public:
	virtual float GetOutputRatio() const = 0;
	virtual ct::fvec2 GetOutputSize() const = 0;
};
