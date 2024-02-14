#pragma once

using namespace std;

#define V1_4_15  0 // 0, supported,		sksevr 2_00_12, vr
#define V1_5_97  1 // 1, supported,		skse64 2_00_20, se
#define	V1_6_318 2 // 2, unsupported,	skse64 2_01_05, ae
#define	V1_6_323 3 // 3, unsupported,	skse64 2_01_05, ae
#define	V1_6_342 4 // 4, unsupported,	skse64 2_01_05, ae
#define	V1_6_353 5 // 5, supported,		skse64 2_01_05, ae
#define	V1_6_629 6 // 6, unsupported,	skse64 2_02_02, ae629+
#define	V1_6_640 7 // 7, supported,		skse64 2_02_02, ae629+
#define V1_6_659 8 // 8, supported,		skse64 2_02_02, ae629+

#if CURRENTVERSION == V1_4_15
#define SKYRIMVR
#endif

#if CURRENTVERSION >= V1_6_318
#define ANNIVERSARY_EDITION
#endif

#if CURRENTVERSION >= V1_6_318 && CURRENTVERSION <= V1_6_353
#define ANNIVERSARY_EDITION_353MINUS
#endif

namespace hdt
{
	namespace offset
	{
	}
}
