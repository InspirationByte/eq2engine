//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Random number generator
//////////////////////////////////////////////////////////////////////////////////

#pragma once

// http://iquilezles.org/www/articles/minispline/minispline.htm
// key format (for DIM == 1) is [t0,x0, t1,x1 ...]
// key format (for DIM == 2) is [t0,x0,y0, t1,x1,y1 ...]
// key format (for DIM == 3) is [t0,x0,y0,z0, t1,x1,y1,z1 ...]
template<int DIM>
void spline(ArrayCRef<float> key, float t, float* v)
{
	static float coefs[16] = {
		-1.0f, 2.0f,-1.0f, 0.0f,
		 3.0f,-5.0f, 0.0f, 2.0f,
		-3.0f, 4.0f, 1.0f, 0.0f,
		 1.0f,-1.0f, 0.0f, 0.0f
	};

	const int size = DIM + 1;

	// find key
	int k = 0;
	while (key[k * size] < t)
		k++;

	ASSERT(k > 0);
	ASSERT(k < key.numElem());

	const float key0 = key[(k - 1) * size];
	const float key1 = key[k * size];

	// interpolant
	const float h = (t - key0) / (key1 - key0);

	// init result
	for (int i = 0; i < DIM; i++)
		v[i] = 0.0f;

	// add basis functions
	for (int i = 0; i < 4; ++i)
	{
		const float* co = &coefs[4 * i];
		const float b = 0.5f * (((co[0] * h + co[1]) * h + co[2]) * h + co[3]);

		const int kn = clamp(k + i - 2, 0, key.numElem() - 1);
		for (int j = 0; j < DIM; j++)
			v[j] += b * key[kn * size + j + 1];
	}
}