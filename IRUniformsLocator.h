#pragma once

#include <glslang/glslang/MachineIndependent/localintermediate.h>
#include <glslang/glslang/Include/InfoSink.h>

#include <unordered_map>
#include <string>

// on va parser le shader
// et tout les uniforms qu'on va rencontrer on va les marquer comme utilis�
// pour le moment on cherche pas � savoir si l'uniforms est vraiment utilis�
// genre si au bout du compte ca valeur ne sert a rien. on verra ca plus tard

class TIRUniformsLocator : public glslang::TIntermTraverser
{
public:
	std::unordered_map<std::string, bool> usedUniforms;

public:
	TIRUniformsLocator();

	virtual bool visitBinary(glslang::TVisit, glslang::TIntermBinary* vNode);
	virtual void visitSymbol(glslang::TIntermSymbol* vNode);
	
protected:
	TIRUniformsLocator(TIRUniformsLocator&);
	TIRUniformsLocator& operator=(TIRUniformsLocator&);
};


