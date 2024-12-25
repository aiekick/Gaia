/*
Copyright 2022-2023 Stephane Cuillerdier (aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once
#pragma warning(disable : 4251)
#pragma warning(disable : 4324)

#include <Gaia/gaia.h>

namespace VertexStruct {
class GAIA_API PipelineVertexInputState {
public:
    vk::PipelineVertexInputStateCreateInfo state = {};
    vk::VertexInputBindingDescription binding = {};
    std::vector<vk::VertexInputAttributeDescription> attributes;
};

// ne pas utiliser size_t e, X64 il utilise des int64 ald des int32 en x86
// win64 => typedef unsigned __int64 size_t;
// win32 => typedef unsigned int     size_t;
// glBufferData supporte les uint mais pas les uint64
// vulkan, il semeble que uint64 verole les indes dans le gpu, est ce uniquement du au binaire x86 ?
// a tester sur x64. vk::DeviceSize est un uint64_t curieusement, mais peut etre que un indexBuffer ne peut supporter ce format
typedef uint32_t I1;

class GAIA_API P3_C4 {
public:
    static void GetInputState(PipelineVertexInputState& vInputState);

public:
    ez::fvec3 p;  // pos
    ez::fvec4 c;  // color

public:
    P3_C4();
    P3_C4(ez::fvec3 vp);
    P3_C4(ez::fvec3 vp, ez::fvec4 vc);
};

class GAIA_API P3_N3_C4 {
public:
    static void GetInputState(PipelineVertexInputState& vInputState);

public:
    ez::fvec3 p;  // pos
    ez::fvec3 n;  // normal
    ez::fvec4 c;  // color

public:
    P3_N3_C4();
    P3_N3_C4(ez::fvec3 vp);
    P3_N3_C4(ez::fvec3 vp, ez::fvec3 vn);
    P3_N3_C4(ez::fvec3 vp, ez::fvec3 vn, ez::fvec4 vc);
};

class GAIA_API P3_N3_C4_D1 {
public:
    static void GetInputState(PipelineVertexInputState& vInputState);

public:
    ez::fvec3 p;     // pos
    ez::fvec3 n;     // normal
    ez::fvec4 c;     // color
    float d = 0.0f;  // distance field

public:
    P3_N3_C4_D1();
    P3_N3_C4_D1(ez::fvec3 vp);
    P3_N3_C4_D1(ez::fvec3 vp, ez::fvec3 vn);
    P3_N3_C4_D1(ez::fvec3 vp, ez::fvec3 vn, ez::fvec4 vc);
    P3_N3_C4_D1(ez::fvec3 vp, ez::fvec3 vn, ez::fvec4 vc, float vd);
};

class GAIA_API P2_T2 {
public:
    static void GetInputState(PipelineVertexInputState& vInputState);

public:
    ez::fvec2 p;  // pos
    ez::fvec2 t;  // tex coord

public:
    P2_T2();
    P2_T2(ez::fvec2 vp);
    P2_T2(ez::fvec2 vp, ez::fvec2 vt);
};

class GAIA_API P3_N3_T2 {
public:
    static void GetInputState(PipelineVertexInputState& vInputState);

public:
    ez::fvec3 p;  // pos
    ez::fvec3 n;  // normal
    ez::fvec2 t;  // tex coord

public:
    P3_N3_T2();
    P3_N3_T2(ez::fvec3 vp);
    P3_N3_T2(ez::fvec3 vp, ez::fvec3 vn);
    P3_N3_T2(ez::fvec3 vp, ez::fvec3 vn, ez::fvec2 vt);
};

class GAIA_API P3_N3_T2_C4 {
public:
    static void GetInputState(PipelineVertexInputState& vInputState);

public:
    ez::fvec3 p;  // pos
    ez::fvec3 n;  // normal
    ez::fvec2 t;  // tex coord
    ez::fvec4 c;  // color

public:
    P3_N3_T2_C4();
    P3_N3_T2_C4(ez::fvec3 vp);
    P3_N3_T2_C4(ez::fvec3 vp, ez::fvec3 vn);
    P3_N3_T2_C4(ez::fvec3 vp, ez::fvec3 vn, ez::fvec2 vt);
    P3_N3_T2_C4(ez::fvec3 vp, ez::fvec3 vn, ez::fvec2 vt, ez::fvec4 vc);
};

class GAIA_API P3_N3_TA3_BTA3_T2_C4 {
public:
    static void GetInputState(PipelineVertexInputState& vInputState);

public:
    ez::fvec3 p;     // pos
    ez::fvec3 n;     // normal
    ez::fvec3 tan;   // tangent
    ez::fvec3 btan;  // bitangent
    ez::fvec2 t;     // tex coord
    ez::fvec4 c;     // color

public:
    P3_N3_TA3_BTA3_T2_C4();
    P3_N3_TA3_BTA3_T2_C4(ez::fvec3 vp);
    P3_N3_TA3_BTA3_T2_C4(ez::fvec3 vp, ez::fvec3 vn);
    P3_N3_TA3_BTA3_T2_C4(ez::fvec3 vp, ez::fvec3 vn, ez::fvec3 vtan);
    P3_N3_TA3_BTA3_T2_C4(ez::fvec3 vp, ez::fvec3 vn, ez::fvec3 vtan, ez::fvec3 vbtan);
    P3_N3_TA3_BTA3_T2_C4(ez::fvec3 vp, ez::fvec3 vn, ez::fvec3 vtan, ez::fvec3 vbtan, ez::fvec2 vt);
    P3_N3_TA3_BTA3_T2_C4(ez::fvec3 vp, ez::fvec3 vn, ez::fvec3 vtan, ez::fvec3 vbtan, ez::fvec2 vt, ez::fvec4 vc);
};
}  // namespace VertexStruct
