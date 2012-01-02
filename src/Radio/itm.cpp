/*****************************************************************************\
 *                                                                           *
 *  The following code was derived from public domain ITM code available     *
 *  at ftp://flattop.its.bldrdoc.gov/itm/ITMDLL.cpp that was released on     *
 *  June 26, 2007.  It was modified to remove Microsoft Windows "dll-isms",  *
 *  redundant and unnecessary #includes, redundant and unnecessary { }'s,    *
 *  to initialize uninitialized variables, type cast some variables,         *
 *  re-format the code for easier reading, and to replace pow() function     *
 *  calls with explicit multiplications wherever possible to increase        *
 *  execution speed and improve computational accuracy.                      *
 *                                                                           *
 *****************************************************************************
 *                                                                           *
 *  Added comments that refer to itm.pdf and itmalg.pdf in a way to easy     *
 *  searching.                                                               *
 *                                                                           *
 * // :0: Blah, page 0     This is the implementation of the code from       *
 *                         itm.pdf, section "0". The description is          *
 *                         found on page 0.                                  *
 * [Alg 0.0]               please refer to algorithm 0.0 from itm_alg.pdf    *
 *                                                                           *
 * Holger Schurig, DH3HS                                                     *
\*****************************************************************************/


// *************************************
// C++ routines for this program are taken from
// a translation of the FORTRAN code written by
// U.S. Department of Commerce NTIA/ITS
// Institute for Telecommunication Sciences
// *****************
// Irregular Terrain Model (ITM) (Longley-Rice)
// *************************************



#include <math.h>
#include <complex>
#include <assert.h>
#include <stdio.h>


using namespace std;

namespace ITM {
	
const double THIRD = (1.0/3.0);
const double f_0 = 47.7; // 47.7 MHz from [Alg 1.1], to convert frequency into wavenumber and vica versa


struct prop_type {
	// General input
	double d;          // distance between the two terminals
	double h_g[2];     // antenna structural heights (above ground)
	double k;          // wave number (= radio frequency)
	double delta_h;    // terrain irregularity parameter
	double N_s;        // minimum monthly surface refractivity, n-Units
	double gamma_e;    // earth's effective curvature
	double Z_g_real;   // surface transfer impedance of the ground
	double Z_g_imag;
	// Additional input for point-to-point mode
	double h_e[2];     // antenna effective heights
	double d_Lj[2];    // horizon distances
	double theta_ej[2];// horizontal elevation angles
	int mdp;           // controlling mode: -1: point to point, 1 start of area, 0 area continuation
        // Output
	int kwx;           // error indicator
	double A_ref;      // reference attenuation
	// used to be propa_type, computed in lrprop()
	double dx;         // scatter distance
	double ael;        // line-of-sight coefficients
	double ak1;        // dito
	double ak2;        // dito
	double aed;        // diffraction coefficients
	double emd;        // dito
	double aes;        // scatter coefficients
	double ems;        // dito
	double d_Lsj[2];   // smooth earth horizon distances
	double d_Ls;       // d_Lsj[] accumulated
	double d_L;        // d_Lj[] accumulated
	double theta_e;    // theta_ej[] accumulated, total bending angle
};


static
int mymin(const int &i, const int &j)
{
	if (i < j)
		return i;
	else
		return j;
}


static
int mymax(const int &i, const int &j)
{
	if (i > j)
		return i;
	else
		return j;
}


static
double mymin(const double &a, const double &b)
{
	if (a < b)
		return a;
	else
		return b;
}


static
double mymax(const double &a, const double &b)
{
	if (a > b)
		return a;
	else
		return b;
}


static
double FORTRAN_DIM(const double &x, const double &y)
{
	// This performs the FORTRAN DIM function.
	// result is x-y if x is greater than y; otherwise result is 0.0

	if (x > y)
		return x - y;
	else
		return 0.0;
}

//#define set_warn(txt, err) printf("%s:%d %s\n", __func__, __LINE__, txt);
#define set_warn(txt, err)

// :13: single-knife attenuation, page 6
/*
 * The attenuation due to a sigle knife edge -- this is an approximation of
 * a Fresnel integral as a function of v^2. The non-approximated integral
 * is documented as [Alg 4.21]
 *
 * Now, what is "single knife attenuation" ? Googling found some paper
 * http://www.its.bldrdoc.gov/pub/ntia-rpt/81-86/81-86.pdf, which actually
 * talks about multi-knife attenuation calculation. However, it says there
 * that single-knife attenuation models attenuation over the edge of one
 * isolated hill.
 *
 * Note that the arguments to this function aren't v, but v^2
 */
static
double Fn(const double &v_square)
{
	double a;

	// [Alg 6.1]
	if (v_square <= 5.76)  // this is the 2.40 from the text, but squared
		a = 6.02 + 9.11 * sqrt(v_square) - 1.27 * v_square;
	else
		a = 12.953 + 4.343 * log(v_square);

	return a;
}


// :14: page 6
/*
 * The heigh-gain over a smooth spherical earth -- to be used in the "three
 * radii" mode. The approximation is that given in in [Alg 6.4ff].
 */
static
double F(const double& x, const double& K)
{
	double w, fhtv;
	if (x <= 200.0) {
		// F = F_2(x, L), which is defined in [Alg 6.6]

		w = -log(K);

		// XXX the text says "or x * w^3 > 450"
		if (K < 1e-5 || (x * w * w * w) > 5495.0) {
			// F_2(x, k) = F_1(x), which is defined in [Alg 6.5]
			// XXX but this isn't the same as in itm_alg.pdf
			fhtv = -117.0;
			if (x > 1.0)
				fhtv = 17.372 * log(x) + fhtv;
		} else {
			// [Alg 6.6], lower part
			fhtv = 2.5e-5 * x * x / K - 8.686 * w - 15.0;
		}
	} else {
		// [Alg 6.3] and [Alg 6.4], lower part, which is G(x)
		fhtv = 0.05751 * x - 4.343 * log(x);

		// [Alg 6.4], middle part, but again XXX this doesn't match totally
		if (x < 2000.0) {
			w = 0.0134 * x * exp(-0.005 * x);
			fhtv = (1.0 - w) * fhtv + w * (17.372 * log(x) - 117.0);
		}
	}

	return fhtv;
}


// :25: Tropospheric scatter frequency gain, [Alg 6.10ff], page 12
static
double H_0(double r, double et)
{
	// constants from [Alg 6.13]
	const double a[5] = {25.0, 80.0, 177.0, 395.0, 705.0};
	const double b[5] = {24.0, 45.0,  68.0,  80.0, 105.0};
	double q, x;
	int it;
	double h0fv;

	it = (int)et;

	if (it <= 0) {
		it = 1;
		q = 0.0;
	} else
	if (it >= 4) {
		it = 4;
		q = 0.0;
	} else {
		q = et - it;
	}

	x = (1.0 / r);
	x *= x;
	// [Alg 6.13], calculates something like H_01(r,j), but not really XXX
	h0fv = 4.343 * log((1.0 + a[it-1] * x + b[it-1]) * x);

	// XXX not sure what this means
	if (q != 0.0)
		h0fv = (1.0 - q) * h0fv + q * 4.343 * log((a[it] * x + b[it]) * x + 1.0);

	return h0fv;
}


// :25: This is the F(\Theta d) function for scatter fields, page 12
static
double F_0(double td)
{
	// [Alg 6.9]
	if (td <= 10e3) // below 10 km
		return 133.4    + 104.6    * td + 71.8     * log(td);
	else
	if (td <= 70e3) // between 10 km and 70 km
		return 0.332e-3 + 0.212e-3 * td + 0.157e-3 * log(td);
	else // above 70 km
		return-4.343    + -1.086   * td + 2.171    * log(td);
}


// :10: Diffraction attenuation, page 4
static
double  adiff(double s, prop_type &prop)
{
	/*
	 * The function adiff finds the "Diffraction attenuation" at the
	 * distance s. It uses a convex combination of smooth earth
	 * diffraction and knife-edge diffraction.
	 */
	static double wd1, xd1, A_fo, qk, aht, xht;
	const double A = 151.03;      // dimensionles constant from [Alg 4.20]
	const double D = 50e3;        // 50 km from [Alg 3.9], scale distance for \delta_h(s)
	const double H = 16;          // 16 m  from [Alg 3.10]
	const double ALPHA = 4.77e-4; // from [Alg 4.10]

	if (s == 0.0) {
		complex<double> prop_zgnd(prop.Z_g_real, prop.Z_g_imag);

		// :11: Prepare initial diffraction constants, page 5
		double q = prop.h_g[0] * prop.h_g[1];
		double qk = prop.h_e[0] * prop.h_e[1] - q;

		if (prop.mdp < 0.0)
			q += 10.0; // "C" from [Alg 4.9]

		// wd1 and xd1 are parts of Q in [Alg 4.10], but I cannot find this there
		wd1 = sqrt(1.0 + qk / q);
		xd1 = prop.d_L + prop.theta_e / prop.gamma_e;  // [Alg 4.9] upper right
		// \delta_h(s), [Alg 3.9]
		q = (1.0 - 0.8 * exp(-prop.d_Ls / D)) * prop.delta_h;
		// \sigma_h(s), [Alg 3.10]
		q *= 0.78 * exp(-pow(q / H, 0.25));

		// A_fo is the "clutter factor"
		A_fo = mymin(15.0, 2.171 * log(1.0 + ALPHA * prop.h_g[0] * prop.h_g[1] * prop.k * q)); // [Alg 4.10]
		// qk is part of the K_j calculation from [Alg 4.17]
		qk = 1.0 / abs(prop_zgnd);
		aht = 20.0; // 20 dB approximation for C_1(K) from [Alg 6.7], see also [Alg 4.25]
		xht = 0.0;

		for (int j = 0; j < 2; ++j) {
			double gamma_j_recip, alpha, K;
			// [Alg 4.15], a is reciproke of gamma_j
			gamma_j_recip = 0.5 * (prop.d_Lj[j] * prop.d_Lj[j]) / prop.h_e[j];
			// [Alg 4.16]
			alpha = pow(gamma_j_recip * prop.k, THIRD);
			// [Alg 4.17]
			K = qk / alpha;
			// [Alg 4.18 and 6.2]
			q = A * (1.607 - K) * alpha * prop.d_Lj[j] / gamma_j_recip;
			// [Alg 4.19, high gain part]
			xht += q;
			// [Alg 4.20] ?,    F(x, k) is in [Alg 6.4]
			aht += F(q, K);
		}
		return 0.0;
	}

	double gamma_o_recip, q, K, ds, theta, alpha, A_r, w;

	// :12: Compute diffraction attenuation, page 5
	// [Alg 4.12]
	theta = prop.theta_e + s * prop.gamma_e;
	// XXX this is not [Alg 4.13]
	ds = s - prop.d_L;
	q = 0.0795775 * prop.k * ds * theta * theta;
	// [Alg 4.14], double knife edge attenuation
	// Note that the arguments to Fn() are not v, but v^2
	double A_k = Fn(q * prop.d_Lj[0] / (ds + prop.d_Lj[0])) +
	             Fn(q * prop.d_Lj[1] / (ds + prop.d_Lj[1]));
	// kinda [Alg 4.15], just so that gamma_o is 1/a
	gamma_o_recip = ds / theta;
	// [Alg 4.16]
	alpha = pow(gamma_o_recip * prop.k, THIRD);
	// [Alg 4.17], note that qk is "1.0 / abs(prop_zgnd)" from above
	K = qk / alpha;
	// [Alg 4.19], q is now X_o
	q = A * (1.607 - K) * alpha * theta + xht;
	// looks a bit like [Alg 4.20], rounded earth attenuation, or??
	// note that G(x) should be "0.05751 * x - 10 * log(q)"
	A_r = 0.05751 * q - 4.343 * log(q) - aht;
	// I'm very unsure if this has anything to do with [Alg 4.9] or not
	q = (wd1 + xd1 / s) * mymin(((1.0 - 0.8 * exp(-s / 50e3)) * prop.delta_h * prop.k), 6283.2);
	// XXX this is NOT the same as the weighting factor from [Alg 4.9]
	w = 25.1 / (25.1 + sqrt(q));
	// [Alg 4.11]
	return (1.0 - w) * A_k + w * A_r + A_fo;
}


/*
 * :22: Scatter attenuation, page 9
 *
 * The function ascat finds the "scatter attenuation" at the distance d. It
 * uses an approximation to the methods of NBS TN101 with check for
 * inadmissable situations. For proper operation, the larger distance (d =
 * d6) must be the first called. A call with d = 0 sets up initial
 * constants.
 *
 * One needs to get TN101, especially chaper 9, to understand this function.
 */
static
double A_scat(double s, prop_type &prop)
{
	static double ad, rr, etq, h0s;

	if (s == 0.0) {
		// :23: Prepare initial scatter constants, page 10
		ad = prop.d_Lj[0] - prop.d_Lj[1];
		rr = prop.h_e[1] / prop.h_e[0];

		if (ad < 0.0) {
			ad = -ad;
			rr = 1.0 / rr;
		}

		etq = (5.67e-6 * prop.N_s - 2.32e-3) * prop.N_s + 0.031; // part of [Alg 4.67]
		h0s = -15.0;
		return 0.0;
	}

	double h0, r1, r2, z0, ss, et, ett, theta_tick, q, temp;

	// :24: Compute scatter attenuation, page 11
	if (h0s > 15.0)
		h0 = h0s;
	else {
		// [Alg 4.61]
		theta_tick = prop.theta_ej[0] + prop.theta_ej[1] + prop.gamma_e * s;
		// [Alg 4.62]
		r2 = 2.0 * prop.k * theta_tick;
		r1 = r2 * prop.h_e[0];
		r2 *= prop.h_e[1];

		if (r1 < 0.2 && r2 < 0.2)
			// The function is undefined
			return 1001.0;

		// XXX not like [Alg 4.65]
		ss = (s - ad) / (s + ad);
		q = rr / ss;
		ss = mymax(0.1, ss);
		q = mymin(mymax(0.1, q), 10.0);
		// XXX not like [Alg 4.66]
		z0 = (s - ad) * (s + ad) * theta_tick * 0.25 / s;
		// [Alg 4.67]
		temp = mymin(1.7, z0 / 8.0e3);
		temp = temp * temp * temp * temp * temp * temp;
		et = (etq * exp(-temp) + 1.0) * z0 / 1.7556e3;

		ett = mymax(et, 1.0);
		h0 = (H_0(r1, ett) + H_0(r2, ett)) * 0.5;  // [Alg 6.12]
		h0 += mymin(h0, (1.38 - log(ett)) * log(ss) * log(q) * 0.49);  // [Alg 6.10 and 6.11]
		h0 = FORTRAN_DIM(h0, 0.0);

		if (et < 1.0)
			// [Alg 6.14]
			h0 = et * h0 + (1.0 - et) * 4.343 * log(pow((1.0 + 1.4142 / r1) * (1.0 + 1.4142 / r2), 2.0) * (r1 + r2) / (r1 + r2 + 2.8284));

		if (h0 > 15.0 && h0s >= 0.0)
			h0 = h0s;
	}

	h0s = h0;
	double theta = prop.theta_e + s * prop.gamma_e;  // [Alg 4.60]
	// [Alg 4.63 and 6.8]
	const double D_0 = 40e3; // 40 km from [Alg 6.8]
	const double H = 47.7;   // 47.7 m from [Alg 4.63]
	return 4.343 * log(prop.k * H * theta * theta * theta * theta)
	       + F_0(theta * s)
	       - 0.1 * (prop.N_s - 301.0) * exp(-theta * s / D_0)
	       + h0;
}


static
double abq_alos(complex<double> r)
{
	return r.real() * r.real() + r.imag() * r.imag();
}


/*
 * :17: line-of-sight attenuation, page 8
 *
 * The function alos finds the "line-of-sight attenuation" at the distance
 * d. It uses a convex combination of plane earth fields and diffracted
 * fields. A call with d=0 sets up initial constants.
 */
static
double A_los(double d, prop_type &prop)
{
	static double wls;

	if (d == 0.0) {
		// :18: prepare initial line-of-sight constants, page 8
		const double D1 = 47.7; // 47.7 m from [Alg 4.43]
		const double D2 = 10e3; // 10 km  from [Alg 4.43]
		const double D1R = 1 / D1;
		// weighting factor w
		wls = D1R / (D1R + prop.k * prop.delta_h / mymax(D2, prop.d_Ls)); // [Alg 4.43]
		return 0;
	}

	complex<double> prop_zgnd(prop.Z_g_real, prop.Z_g_imag);
	complex<double> r;
	double s, sps, q;

	// :19: compute line of sight attentuation, page 8
	const double D = 50e3; // 50 km from [Alg 3.9]
	const double H = 16.0; // 16 m  from [Alg 3.10]
	q = (1.0 - 0.8 * exp(-d / D)) * prop.delta_h;     // \Delta h(d), [Alg 3.9]
	s = 0.78 * q * exp(-pow(q / H, 0.25));       // \sigma_h(d), [Alg 3.10]
	q = prop.h_e[0] + prop.h_e[1];
	sps = q / sqrt(d * d + q * q);               // sin(\psi), [Alg 4.46]
	r = (sps - prop_zgnd) / (sps + prop_zgnd) * exp(-mymin(10.0, prop.k * s * sps)); // [Alg 4.47]
	q = abq_alos(r);

	if (q < 0.25 || q < sps)                     // [Alg 4.48]
		r = r * sqrt(sps / q);

	double alosv = prop.emd * d + prop.aed;    // [Alg 4.45]
	q = prop.k * prop.h_e[0] * prop.h_e[1] * 2.0 / d; // [Alg 4.49]

	// M_PI is pi, M_PI_2 is pi/2
	if (q > M_PI_2)                              // [Alg 4.50]
		q = M_PI - (M_PI_2 * M_PI_2) / q;

	// [Alg 4.51 and 4.44]
	return (-4.343 * log(abq_alos(complex<double>(cos(q), -sin(q)) + r)) - alosv) * wls + alosv;
}


// :5: LRprop, page 2
/*
 * The value mdp controls some of the program flow. When it equals -1 we are
 * in point-to-point mode, when 1 we are beginning the area mode, and when 0
 * we are continuing the area mode. The assumption is that when one uses the
 * area mode, one will want a sequence of results for varying distances.
 */
static
void lrprop(double d, prop_type &prop)
{
	static bool wlos, wscat;
	static double dmin, xae;
	complex<double> prop_zgnd(prop.Z_g_real, prop.Z_g_imag);
	double a0, a1, a2, a3, a4, a5, a6;
	double d0, d1, d2, d3, d4, d5, d6;
	bool wq;
	double q;
	int j;


	if (prop.mdp != 0) {
		// :6: Do secondary parameters, page 3
		// [Alg 3.5]
		for (j = 0; j < 2; j++)
			prop.d_Lsj[j] = sqrt(2.0 * prop.h_e[j] / prop.gamma_e);

		// [Alg 3.6]
		prop.d_Ls = prop.d_Lsj[0] + prop.d_Lsj[1];
		// [Alg 3.7]
		prop.d_L = prop.d_Lj[0] + prop.d_Lj[1];
		// [Alg 3.8]
		prop.theta_e = mymax(prop.theta_ej[0] + prop.theta_ej[1], -prop.d_L * prop.gamma_e);
		wlos = false;
		wscat = false;

		// :7: Check parameters range, page 3

		// kwx is some kind of error indicator. Setting kwx to 1
		// means "results are slightly bad", while setting it to 4
		// means "my calculations will be bogus"

		// Frequency must be between 40 MHz and 10 GHz
		// Wave number (wn) = 2 * M_PI / lambda, or f/f0, where f0 = 47.7 MHz*m;
		// 0.838 => 40 MHz, 210 => 10 GHz
		if (prop.k < 0.838 || prop.k > 210.0) {
			set_warn("Frequency not optimal", 1);
			prop.kwx = mymax(prop.kwx, 1);
		}

		// Surface refractivity, see [Alg 1.2]
		if (prop.N_s < 250.0 || prop.N_s > 400.0) {
			set_warn("Surface refractivity out-of-bounds", 4);
			prop.kwx = 4;
		} else
		// Earth's effective curvature, see [Alg 1.3]
		if (prop.gamma_e < 75e-9 || prop.gamma_e > 250e-9) {
			set_warn("Earth's curvature out-of-bounds", 4);
			prop.kwx = 4;
		} else
		// Surface transfer impedance, see [Alg 1.4]
		if (prop_zgnd.real() <= abs(prop_zgnd.imag())) {
			set_warn("Surface transfer impedance out-of-bounds", 4);
			prop.kwx = 4;
		} else
		// Calulating outside of 20 MHz to 40 GHz is really bad
		if (prop.k < 0.419 || prop.k > 420.0) {
			set_warn("Frequency out-of-bounds", 4);
			prop.kwx = 4;
		} else
		for (j = 0; j < 2; j++) {
			// Antenna structural height should be between 1 and 1000 m
			if (prop.h_g[j] < 1.0 || prop.h_g[j] > 1000.0) {
				set_warn("Antenna height not optimal", 1);
				prop.kwx = mymax(prop.kwx, 1);
			}

			// Horizon elevation angle
			if (abs(prop.theta_ej[j]) > 200e-3) {
				set_warn("Horizon elevation weird", 3);
				prop.kwx = mymax(prop.kwx, 3);
			}

			// Horizon distance dl,
			// Smooth earth horizon distance dls
			if (prop.d_Lj[j] < 0.1 * prop.d_Lsj[j] ||
			    prop.d_Lj[j] > 3.0 * prop.d_Lsj[j]) {
				set_warn("Horizon distance weird", 3);
				prop.kwx = mymax(prop.kwx, 3);
			}
			// Antenna structural height must between  0.5 and 3000 m
			if (prop.h_g[j] < 0.5 || prop.h_g[j] > 3000.0) {
				set_warn("Antenna heights out-of-bounds", 4);
				prop.kwx = 4;
			}
		}

		dmin = abs(prop.h_e[0] - prop.h_e[1]) / 200e-3;

		// :9: Diffraction coefficients, page 4
		/*
		 * This is the region beyond the smooth-earth horizon at
		 * D_Lsa and short of where isotropic scatter takes over. It
		 * is a key to central region and associated coefficients
		 * must always be computed.
		 */
		q = adiff(0.0, prop);
		xae = pow(prop.k * prop.gamma_e * prop.gamma_e, -THIRD);  // [Alg 4.2]
		d3 = mymax(prop.d_Ls, 1.3787 * xae + prop.d_L);    // [Alg 4.3]
		d4 = d3 + 2.7574 * xae;                            // [Alg 4.4]
		a3 = adiff(d3, prop);                              // [Alg 4.5]
		a4 = adiff(d4, prop);                              // [Alg 4.7]

		prop.emd = (a4 - a3) / (d4 - d3);                  // [Alg 4.8]
		prop.aed = a3 - prop.emd * d3;
	}

	if (prop.mdp >= 0) {
		prop.mdp = 0;
		prop.d = d;
	}

	if (prop.d > 0.0) {
		// :8: Check distance, page 3

		// Distance above 1000 km is guessing
		if (prop.d > 1000e3) {
			set_warn("Distance not optimal", 1);
			prop.kwx = mymax(prop.kwx, 1);
		}

		// Distance too small, use some indoor algorithm :-)
		if (prop.d < dmin) {
			set_warn("Distance too small", 3);
			prop.kwx = mymax(prop.kwx, 3);
		}

		// Distance above 2000 km is really bad, don't do that
		if (prop.d < 1e3 || prop.d > 2000e3) {
			set_warn("Distance out-of-bounds", 4);
			prop.kwx = 4;
		}
	}

	if (prop.d < prop.d_Ls) {
		// :15: Line-of-sight calculations, page 7
		if (!wlos) {
			// :16: Line-of-sight coefficients, page 7
			q = A_los(0.0, prop);
			d2 = prop.d_Ls;
			a2 = prop.aed + d2 * prop.emd;
			d0 = 1.908 * prop.k * prop.h_e[0] * prop.h_e[1];                // [Alg 4.38]

			if (prop.aed >= 0.0) {
				d0 = mymin(d0, 0.5 * prop.d_L);                       // [Alg 4.28]
				d1 = d0 + 0.25 * (prop.d_L - d0);                     // [Alg 4.29]
			} else {
				d1 = mymax(-prop.aed / prop.emd, 0.25 * prop.d_L);  // [Alg 4.39]
			}

			a1 = A_los(d1, prop);  // [Alg 4.31]
			wq = false;

			if (d0 < d1) {
				a0 = A_los(d0, prop); // [Alg 4.30]
				q = log(d2 / d0);
				prop.ak2 = mymax(0.0, ((d2 - d0) * (a1 - a0) - (d1 - d0) * (a2 - a0)) / ((d2 - d0) * log(d1 / d0) - (d1 - d0) * q)); // [Alg 4.32]
				wq = prop.aed >= 0.0 || prop.ak2 > 0.0;

				if (wq) {
					prop.ak1 = (a2 - a0 - prop.ak2 * q) / (d2 - d0); // [Alg 4.33]

					if (prop.ak1 < 0.0) {
						prop.ak1 = 0.0;                      // [Alg 4.36]
						prop.ak2 = FORTRAN_DIM(a2, a0) / q;  // [Alg 4.35]

						if (prop.ak2 == 0.0)                 // [Alg 4.37]
							prop.ak1 = prop.emd;
					}
				}
			}

			if (!wq) {
				prop.ak1 = FORTRAN_DIM(a2, a1) / (d2 - d1);   // [Alg 4.40]
				prop.ak2 = 0.0;                               // [Alg 4.41]

				if (prop.ak1 == 0.0)                          // [Alg 4.37]
					prop.ak1 = prop.emd;
			}

			prop.ael = a2 - prop.ak1 * d2 - prop.ak2 * log(d2); // [Alg 4.42]
			wlos = true;
		}

		if (prop.d > 0.0)
			// [Alg 4.1]
			/*
			 * The reference attenuation is determined as a function of the distance
			 * d from 3 piecewise formulatios. This is part one.
			 */
			prop.A_ref = prop.ael + prop.ak1 * prop.d + prop.ak2 * log(prop.d);
	}

	if (prop.d <= 0.0 || prop.d >= prop.d_Ls) {
		// :20: Troposcatter calculations, page 9
		if (!wscat) {
			// :21: Troposcatter coefficients
			const double DS = 200e3;             // 200 km from [Alg 4.53]
			const double HS = 47.7;              // 47.7 m from [Alg 4.59]
			q = A_scat(0.0, prop);
			d5 = prop.d_L + DS;                  // [Alg 4.52]
			d6 = d5 + DS;                        // [Alg 4.53]
			a6 = A_scat(d6, prop);                // [Alg 4.54]
			a5 = A_scat(d5, prop);                // [Alg 4.55]

			if (a5 < 1000.0) {
				prop.ems = (a6 - a5) / DS;   // [Alg 4.57]
				prop.dx = mymax(prop.d_Ls,   // [Alg 4.58]
				                 mymax(prop.d_L + 0.3 * xae * log(HS * prop.k),
				                 (a5 - prop.aed - prop.ems * d5) / (prop.emd - prop.ems)));
				prop.aes = (prop.emd - prop.ems) * prop.dx + prop.aed; // [Alg 4.59]
			} else {
				prop.ems = prop.emd;
				prop.aes = prop.aed;
				prop.dx = 10.e6;             // [Alg 4.56]
			}
			wscat = true;
		}

		// [Alg 4.1], part two and three.
		if (prop.d > prop.dx)
			prop.A_ref = prop.aes + prop.ems * prop.d;  // scatter region
		else
			prop.A_ref = prop.aed + prop.emd * prop.d;  // diffraction region
	}

	prop.A_ref = mymax(prop.A_ref, 0.0);
}


/*****************************************************************************/

// :27: Variablility parameters, page 13

struct propv_type {
	// Input:
	int lvar;    // control switch for initialisation and/or recalculation
	             // 0: no recalculations needed
	             // 1: distance changed
	             // 2: antenna heights changed
	             // 3: frequency changed
	             // 4: mdvar changed
	             // 5: climate changed, or initialize everything
	int mdvar;   // desired mode of variability
	int klim;    // climate indicator
	// Output
	double sgc;  // standard deviation of situation variability (confidence)
};


#ifdef WITH_QRLA
// :40: Prepare model for area prediction mode, page 21
/*
 * This is used to prepare the model in the area prediction mode. Normally,
 * one first calls qlrps() and then qlra(). Before calling the latter, one
 * should have defined in prop_type the antenna heights @hg, the terrain
 * irregularity parameter @dh , and (probably through qlrps() the variables
 * @wn, @ens, @gme, and @zgnd. The input @kst will define siting criteria
 * for the terminals, @klimx the climate, and @mdvarx the mode of
 * variability. If @klimx <= 0 or @mdvarx < 0 the associated parameters
 * remain unchanged.
 */
static
void qlra(int kst[], int klimx, int mdvarx, prop_type &prop, propv_type &propv)
{
	double q;

	for (int j = 0; j < 2; ++j) {
		if (kst[j] <= 0)
			// [Alg 3.1]
			prop.h_e[j] = prop.h_g[j];
		else {
			q = 4.0;

			if (kst[j] != 1)
				q = 9.0;

			if (prop.h_g[j] < 5.0)
				q *= sin(0.3141593 * prop.h_g[j]);

			prop.h_e[j] = prop.h_g[j] + (1.0 + q) * exp(-mymin(20.0, 2.0 * prop.h_g[j] / mymax(1e-3, prop.delta_h)));
		}

		// [Alg 3.3], upper function. q is d_Ls_j
		const double H_3 = 5; // 5m from [Alg 3.3]
		q = sqrt(2.0 * prop.h_e[j] / prop.gamma_e);
		prop.d_Lj[j] = q * exp(-0.07 * sqrt(prop.delta_h / mymax(prop.h_e[j], H_3)));
		// [Alg 3.4]
		prop.theta_ej[j] = (0.65 * prop.delta_h * (q / prop.d_Lj[j] - 1.0) - 2.0 * prop.h_e[j]) / q;
	}

	prop.mdp = 1;
	propv.lvar = mymax(propv.lvar, 3);

	if (mdvarx >= 0) {
		propv.mdvar = mdvarx;
		propv.lvar = mymax(propv.lvar, 4);
	}

	if (klimx > 0) {
		propv.klim = klimx;
		propv.lvar = 5;
	}
}
#endif


// :51: Inverse of standard normal complementary probability
/*
 * The approximation is due to C. Hastings, Jr. ("Approximations for digital
 * computers," Princeton Univ. Press, 1955) and the maximum error should be
 * 4.5e-4.
 */
#if 1
static
double qerfi(double q)
{
	double x, t, v;
	const double c0 = 2.515516698;
	const double c1 = 0.802853;
	const double c2 = 0.010328;
	const double d1 = 1.432788;
	const double d2 = 0.189269;
	const double d3 = 0.001308;

	x = 0.5 - q;
	t = mymax(0.5 - fabs(x), 0.000001);
	t = sqrt(-2.0 * log(t));
	v = t - ((c2 * t + c1) * t + c0) / (((d3 * t + d2) * t + d1) * t + 1.0);

	if (x < 0.0)
		v = -v;

	return v;
}
#endif


// :41: preparatory routine, page 20
/*
 * This subroutine converts
 *   frequency @fmhz
 *   surface refractivity reduced to sea level @en0
 *   general system elevation @zsys
 *   polarization and ground constants @eps, @sgm
 * to
 *   wave number @wn,
 *   surface refractivity @ens
 *   effective earth curvature @gme
 *   surface impedance @zgnd
 * It may be used with either the area prediction or the point-to-point mode.
 */
#if 1
static
void qlrps(double fmhz, double zsys, double en0, int ipol, double eps, double sgm, prop_type &prop)

{
	const double Z_1 = 9.46e3; // 9.46 km       from [Alg 1.2]
	const double gma = 157e-9; // 157e-9 1/m    from [Alg 1.3]
	const double N_1 = 179.3;  // 179.3 N-units from [Alg 1.3]
	const double Z_0 = 376.62; // 376.62 Ohm    from [Alg 1.5]

	// Frequecy -> Wave number
	prop.k = fmhz / f_0;                                      // [Alg 1.1]

	// Surface refractivity
	prop.N_s = en0;
	if (zsys != 0.0)
		prop.N_s *= exp(-zsys / Z_1);                      // [Alg 1.2]

	// Earths effective curvature
	prop.gamma_e = gma * (1.0 - 0.04665 * exp(prop.N_s / N_1));    // [Alg 1.3]

	// Surface transfer impedance
	complex<double> zq, prop_zgnd(prop.Z_g_real, prop.Z_g_imag);
	zq = complex<double> (eps, Z_0 * sgm / prop.k);        // [Alg 1.5]
	prop_zgnd = sqrt(zq - 1.0);

	// adjust surface transfer impedance for Polarization
	if (ipol != 0.0)
		prop_zgnd = prop_zgnd / zq;                        // [Alg 1.4]

	prop.Z_g_real = prop_zgnd.real();
	prop.Z_g_imag = prop_zgnd.imag();
}
#endif


// :30: Function curv, page 15
static
double curve(double const &c1, double const &c2, double const &x1, double const &x2, double const &x3, double const &de)
{
	double temp1, temp2;

	temp1 = (de - x2) / x3;
	temp2 = de / x1;

	temp1 *= temp1;
	temp2 *= temp2;

	return (c1 + c2 / (1.0 + temp1)) * temp2 / (1.0 + temp2);
}


// :28: Area variablity, page 14
#if 1
static
double avar(double zzt, double zzl, double zzc, prop_type &prop, propv_type &propv)
{
	static int kdv;
	static double dexa, de, vmd, vs0, sgl, sgtm, sgtp, sgtd, tgtd, gm, gp, cv1, cv2, yv1, yv2, yv3, csm1, csm2, ysm1, ysm2, ysm3, csp1, csp2, ysp1, ysp2, ysp3, csd1, zd, cfm1, cfm2, cfm3, cfp1, cfp2, cfp3;

	// :29: Climatic constants, page 15
	// Indexes are:
	//   0: equatorial
	//   1: continental suptropical
	//   2: maritime subtropical
	//   3: desert
	//   4: continental temperature
	//   5: maritime over land
	//   6: maritime over sea
	//                      equator  contsup maritsup   desert conttemp mariland  marisea
	const double bv1[7]  = {  -9.67,   -0.62,    1.26,   -9.21,   -0.62,   -0.39,    3.15};
	const double bv2[7]  = {   12.7,    9.19,    15.5,    9.05,    9.19,    2.86,   857.9};
	const double xv1[7]  = {144.9e3, 228.9e3, 262.6e3,  84.1e3, 228.9e3, 141.7e3, 2222.e3};
	const double xv2[7]  = {190.3e3, 205.2e3, 185.2e3, 101.1e3, 205.2e3, 315.9e3, 164.8e3};
	const double xv3[7]  = {133.8e3, 143.6e3,  99.8e3,  98.6e3, 143.6e3, 167.4e3, 116.3e3};
	const double bsm1[7] = {   2.13,    2.66,    6.11,    1.98,    2.68,    6.86,    8.51};
	const double bsm2[7] = {  159.5,    7.67,    6.65,   13.11,    7.16,   10.38,   169.8};
	const double xsm1[7] = {762.2e3, 100.4e3, 138.2e3, 139.1e3,  93.7e3, 187.8e3, 609.8e3};
	const double xsm2[7] = {123.6e3, 172.5e3, 242.2e3, 132.7e3, 186.8e3, 169.6e3, 119.9e3};
	const double xsm3[7] = { 94.5e3, 136.4e3, 178.6e3, 193.5e3, 133.5e3, 108.9e3, 106.6e3};
	const double bsp1[7] = {   2.11,    6.87,   10.08,    3.68,    4.75,    8.58,    8.43};
	const double bsp2[7] = {  102.3,   15.53,    9.60,   159.3,    8.12,   13.97,    8.19};
	const double xsp1[7] = {636.9e3, 138.7e3, 165.3e3, 464.4e3,  93.2e3, 216.0e3, 136.2e3};
	const double xsp2[7] = {134.8e3, 143.7e3, 225.7e3,  93.1e3, 135.9e3, 152.0e3, 188.5e3};
	const double xsp3[7] = { 95.6e3,  98.6e3, 129.7e3,  94.2e3, 113.4e3, 122.7e3, 122.9e3};
	// bds1 -> is similar to C_D from table 5.1 at [Alg 5.8]
	// bzd1 -> is similar to z_D from table 5.1 at [Alg 5.8]
	const double bsd1[7] = {  1.224,   0.801,   1.380,   1.000,   1.224,   1.518,   1.518};
	const double bzd1[7] = {  1.282,   2.161,   1.282,    20.0,   1.282,   1.282,   1.282};
	const double bfm1[7] = {    1.0,     1.0,     1.0,     1.0,    0.92,     1.0,    1.0};
	const double bfm2[7] = {    0.0,     0.0,     0.0,     0.0,    0.25,     0.0,    0.0};
	const double bfm3[7] = {    0.0,     0.0,     0.0,     0.0,    1.77,     0.0,    0.0};
	const double bfp1[7] = {    1.0,    0.93,     1.0,    0.93,    0.93,     1.0,    1.0};
	const double bfp2[7] = {    0.0,    0.31,     0.0,    0.19,    0.31,     0.0,    0.0};
	const double bfp3[7] = {    0.0,    2.00,     0.0,    1.79,    2.00,     0.0,    0.0};
	const double rt = 7.8, rl = 24.0;
	static bool no_location_variability, no_situation_variability;
	double avarv, q, vs, zt, zl, zc;
	double sgt, yr;
	int temp_klim;

	if (propv.lvar > 0) {
		// :31: Setup variablity constants, page 16
		switch (propv.lvar) {
		default:
			// Initialization or climate change

			// if climate is wrong, use some "continental temperature" as default
			// and set error indicator
			if (propv.klim <= 0 || propv.klim > 7) {
				propv.klim = 5;
				prop.kwx = mymax(prop.kwx, 2);
				set_warn("Climate index set to continental", 2);
			}

			// convert climate number into index into the climate tables
			temp_klim = propv.klim - 1;

			// :32: Climatic coefficients, page 17
			cv1 = bv1[temp_klim];
			cv2 = bv2[temp_klim];
			yv1 = xv1[temp_klim];
			yv2 = xv2[temp_klim];
			yv3 = xv3[temp_klim];
			csm1 = bsm1[temp_klim];
			csm2 = bsm2[temp_klim];
			ysm1 = xsm1[temp_klim];
			ysm2 = xsm2[temp_klim];
			ysm3 = xsm3[temp_klim];
			csp1 = bsp1[temp_klim];
			csp2 = bsp2[temp_klim];
			ysp1 = xsp1[temp_klim];
			ysp2 = xsp2[temp_klim];
			ysp3 = xsp3[temp_klim];
			csd1 = bsd1[temp_klim];
			zd = bzd1[temp_klim];
			cfm1 = bfm1[temp_klim];
			cfm2 = bfm2[temp_klim];
			cfm3 = bfm3[temp_klim];
			cfp1 = bfp1[temp_klim];
			cfp2 = bfp2[temp_klim];
			cfp3 = bfp3[temp_klim];
			// fall throught

		case 4:
			// :33: Mode of variablity coefficients, page 17

			// This code means that propv.mdvar can be
			//  0 ..  3
			// 10 .. 13, then no_location_variability is set              (no location variability)
			// 20 .. 23, then no_situation_variability is set             (no situatian variability)
			// 30 .. 33, then no_location_variability and no_situation_variability is set
			kdv = propv.mdvar;
			no_situation_variability = kdv >= 20;
			if (no_situation_variability)
				kdv -= 20;

			no_location_variability = kdv >= 10;
			if (no_location_variability)
				kdv -= 10;

			if (kdv < 0 || kdv > 3) {
				kdv = 0;
				set_warn("kdv set to 0", 2);
				prop.kwx = mymax(prop.kwx, 2);
			}

			// fall throught

		case 3:
			// :34: Frequency coefficients, page 18
			q = log(0.133 * prop.k);
			gm = cfm1 + cfm2 / ((cfm3 * q * cfm3 * q) + 1.0);
			gp = cfp1 + cfp2 / ((cfp3 * q * cfp3 * q) + 1.0);
			// fall throught

		case 2:
			// :35: System coefficients, page 18
			// [Alg 5.3], effective distance
			{
			const double a_1 = 9000e3; // 9000 km from [[Alg 5.3]
			//XXX I don't have any idea how they made up the third summand,
			//XXX text says    a_1 * pow(k * D_1, -THIRD)
			//XXX with const double D_1 = 1266; // 1266 km
			dexa = sqrt(2*a_1 * prop.h_e[0]) +
			       sqrt(2*a_1 * prop.h_e[1]) +
			       pow((575.7e12 / prop.k), THIRD);
			}
			// fall throught

		case 1:
			// :36: Distance coefficients, page 18
			{
			// [Alg 5.4]
			const double D_0 = 130e3; // 130 km from [Alg 5.4]
			if (prop.d < dexa)
				de = D_0 * prop.d / dexa;
			else
				de = D_0 + prop.d - dexa;
			}
		}
		/*
		 * Quantiles of time variability are computed using a
		 * variation of the methods described in Section 10 and
		 * Annex III.7 of NBS~TN101, and also in CCIR
		 * Report {238-3}. Those methods speak of eight or nine
		 * discrete radio climates, of which seven have been
		 * documented with corresponding empirical curves. It is
		 * these empirical curves to which we refer below. They are
		 * all curves of quantiles of deviations versus the
		 * effective distance @de.
		 */
		vmd = curve(cv1, cv2, yv1, yv2, yv3, de);             // [Alg 5.5]
		// [Alg 5.7], the slopes or "pseudo-standard deviations":
		// sgtm -> \sigma T-
		// sgtp -> \sigma T+
		sgtm = curve(csm1, csm2, ysm1, ysm2, ysm3, de) * gm;
		sgtp = curve(csp1, csp2, ysp1, ysp2, ysp3, de) * gp;
		// [Alg 5.8], ducting, "sgtd" -> \sigma TD
		sgtd = sgtp * csd1;
		tgtd = (sgtp - sgtd) * zd;

		// Location variability
		if (no_location_variability) {
			sgl = 0.0;
		} else {
			// Alg [3.9]
			q = (1.0 - 0.8 * exp(-prop.d / 50e3)) * prop.delta_h * prop.k;
			// [Alg 5.9]
			sgl = 10.0 * q / (q + 13.0);
		}

		// Situation variability
		if (no_situation_variability) {
			vs0 = 0.0;
		} else {
			const double D = 100e3; // 100 km
			vs0 = (5.0 + 3.0 * exp(-de / D));         // [Alg 5.10]
			vs0 *= vs0;
		}

		// Mark all constants as initialized
		propv.lvar = 0;
	}


	// :37: Correct normal deviates, page 19
	zt = zzt;
	zl = zzl;
	zc = zzc;
	// kdv is derived from prop.mdvar
	switch (kdv) {
	case 0:
		// Single message mode. Time, location and situation variability
		// are combined to form a confidence level.
		zt = zc;
		zl = zc;
		break;

	case 1:
		// Individual mode. Reliability is given by time
		// variability. Confidence. is a combination of location
		// and situation variability.
		zl = zc;
		break;

	case 2:
		// Mobile modes. Reliability is a combination of time and
		// location variability. Confidence. is given by the
		// situation variability.
		zl = zt;
		// case 3: Broadcast mode. like avar(zt, zl, zc).
		// Reliability is given by the two-fold statement of at
		// least qT of the time in qL of the locations. Confidence.
		// is given by the situation variability.
	}
	if (fabs(zt) > 3.1 || fabs(zl) > 3.1 || fabs(zc) > 3.1) {
		set_warn("Situations variables not optimal", 1);
		prop.kwx = mymax(prop.kwx, 1);
	}


	// :38: Resolve standard deviations, page 19
	if (zt < 0.0)
		sgt = sgtm;
	else
	if (zt <= zd)
		sgt = sgtp;
	else
		sgt = sgtd + tgtd / zt;
	// [Alg 5.11], situation variability
	vs = vs0 + (sgt * zt * sgt * zt) / (rt + zc * zc) + (sgl * zl * sgl * zl) / (rl + zc * zc);


	// :39: Resolve deviations, page 19
	if (kdv == 0) {
		yr = 0.0;
		propv.sgc = sqrt(sgt * sgt + sgl * sgl + vs);
	} else
	if (kdv == 1) {
		yr = sgt * zt;
		propv.sgc = sqrt(sgl * sgl + vs);
	} else
	if (kdv == 2) {
		yr = sqrt(sgt * sgt + sgl * sgl) * zt;
		propv.sgc = sqrt(vs);
	} else {
		yr = sgt * zt + sgl * zl;
		propv.sgc = sqrt(vs);
	}

	// [Alg 5.1], area variability
	avarv = prop.A_ref - vmd - yr - propv.sgc * zc;

	// [Alg 5.2]
	if (avarv < 0.0)
		avarv = avarv * (29.0 - avarv) / (29.0 - 10.0 * avarv);

	return avarv;
}
#endif

// :45: Find to horizons, page 24
/*
 * Here we use the terrain profile @pfl to find the two horizons. Output
 * consists of the horizon distances @dl and the horizon take-off angles
 * @the. If the path is line-of-sight, the routine sets both horizon
 * distances equal to @dist.
 */
static
void hzns(double pfl[], prop_type &prop)
{
	bool wq;
	int np;
	double xi, za, zb, qc, q, sb, sa;

	np = (int)pfl[0];
	xi = pfl[1];
	za = pfl[2] + prop.h_g[0];
	zb = pfl[np+2] + prop.h_g[1];
	qc = 0.5 * prop.gamma_e;
	q = qc * prop.d;
	prop.theta_ej[1] = (zb - za) / prop.d;
	prop.theta_ej[0] = prop.theta_ej[1] - q;
	prop.theta_ej[1] = -prop.theta_ej[1] - q;
	prop.d_Lj[0] = prop.d;
	prop.d_Lj[1] = prop.d;

	if (np >= 2) {
		sa = 0.0;
		sb = prop.d;
		wq = true;

		for (int i = 1; i < np; i++) {
			sa += xi;
			sb -= xi;
			q = pfl[i+2] - (qc * sa + prop.theta_ej[0]) * sa - za;

			if (q > 0.0) {
				prop.theta_ej[0] += q / sa;
				prop.d_Lj[0] = sa;
				wq = false;
			}

			if (!wq) {
				q = pfl[i+2] - (qc * sb + prop.theta_ej[1]) * sb - zb;

				if (q > 0.0) {
					prop.theta_ej[1] += q / sb;
					prop.d_Lj[1] = sb;
				}
			}
		}
	}
}


// :53: Linear least square fit, page 28
static
void zlsq1(double z[], const double &x1, const double &x2, double& z0, double& zn)

{
	double xn, xa, xb, x, a, b;
	int n, ja, jb;

	xn = z[0];
	xa = int(FORTRAN_DIM(x1 / z[1], 0.0));
	xb = xn - int(FORTRAN_DIM(xn, x2 / z[1]));

	if (xb <= xa) {
		xa = FORTRAN_DIM(xa, 1.0);
		xb = xn - FORTRAN_DIM(xn, xb + 1.0);
	}

	ja = (int)xa;
	jb = (int)xb;
	n = jb - ja;
	xa = xb - xa;
	x = -0.5 * xa;
	xb += x;
	a = 0.5 * (z[ja+2] + z[jb+2]);
	b = 0.5 * (z[ja+2] - z[jb+2]) * x;

	for (int i = 2; i <= n; ++i) {
		++ja;
		x += 1.0;
		a += z[ja+2];
		b += z[ja+2] * x;
	}

	a /= xa;
	b = b * 12.0 / ((xa * xa + 2.0) * xa);
	z0 = a - b * xb;
	zn = a + b * (xn - xb);
}


// :52: Provide a quantile and reorders array @a, page 27
static
double qtile(const int &nn, double a[], const int &ir)
{
	double q = 0.0, r;                   /* Initializations by KD2BD */
	int m, n, i, j, j1 = 0, i0 = 0, k;   /* Initializations by KD2BD */
	bool done = false;
	bool goto10 = true;

	m = 0;
	n = nn;
	k = mymin(mymax(0, ir), n);

	while (!done) {
		if (goto10) {
			q = a[k];
			i0 = m;
			j1 = n;
		}

		i = i0;

		while (i <= n && a[i] >= q)
			i++;

		if (i > n)
			i = n;
		j = j1;

		while (j >= m && a[j] <= q)
			j--;

		if (j < m)
			j = m;

		if (i < j) {
			r = a[i];
			a[i] = a[j];
			a[j] = r;
			i0 = i + 1;
			j1 = j - 1;
			goto10 = false;
		} else
		if (i < k) {
			a[k] = a[i];
			a[i] = q;
			m = i + 1;
			goto10 = true;
		} else
		if (j > k) {
			a[k] = a[j];
			a[j] = q;
			n = j - 1;
			goto10 = true;
		} else {
			done = true;
		}
	}

	return q;
}


#ifdef WITH_QERF
// :50: Standard normal complementary probability, page 26
static
double qerf(const double &z)
{
	const double b1 = 0.319381530, b2 = -0.356563782, b3 = 1.781477937;
	const double b4 = -1.821255987, b5 = 1.330274429;
	const double rp = 4.317008, rrt2pi = 0.398942280;
	double t, x, qerfv;

	x = z;
	t = fabs(x);

	if (t >= 10.0)
		qerfv = 0.0;
	else {
		t = rp / (t + rp);
		qerfv = exp(-0.5 * x * x) * rrt2pi * ((((b5 * t + b4) * t + b3) * t + b2) * t + b1) * t;
	}

	if (x < 0.0)
		qerfv = 1.0 - qerfv;

	return qerfv;
}
#endif


// :48: Find interdecile range of elevations, page 25
/*
 * Using the terrain profile @pfl we find \Delta h, the interdecile range of
 * elevations between the two points @x1 and @x2.
 */
static
double dlthx(double pfl[], const double &x1, const double &x2)
{
	int np, ka, kb, n, k, j;
	double dlthxv, sn, xa, xb;
	double *s;

	np = (int)pfl[0];
	xa = x1 / pfl[1];
	xb = x2 / pfl[1];
	dlthxv = 0.0;

	if (xb - xa < 2.0) // exit out
		return dlthxv;

	ka = (int)(0.1 * (xb - xa + 8.0));
	ka = mymin(mymax(4, ka), 25);
	n = 10 * ka - 5;
	kb = n - ka + 1;
	sn = n - 1;
	s = new double[n+2];
	s[0] = sn;
	s[1] = 1.0;
	xb = (xb - xa) / sn;
	k = (int)(xa + 1.0);
	xa -= (double)k;

	for (j = 0; j < n; j++) {
		// Reduce
		while (xa > 0.0 && k < np) {
			xa -= 1.0;
			++k;
		}

		s[j+2] = pfl[k+2] + (pfl[k+2] - pfl[k+1]) * xa;
		xa = xa + xb;
	}

	zlsq1(s, 0.0, sn, xa, xb);
	xb = (xb - xa) / sn;

	for (j = 0; j < n; j++) {
		s[j+2] -= xa;
		xa = xa + xb;
	}

	dlthxv = qtile(n - 1, s + 2, ka - 1) - qtile(n - 1, s + 2, kb - 1);
	dlthxv /= 1.0 - 0.8 * exp(-(x2 - x1) / 50.0e3);
	delete[] s;

	return dlthxv;
}


// :41: Prepare model for point-to-point operation, page 22
/*
 * This mode requires the terrain profile lying between the terminals. This
 * should be a sequence of surface elevations at points along the great
 * circle path joining the two points. It should start at the ground beneath
 * the first terminal and end at the ground beneath the second. In the
 * present routine it is assumed that the elevations are equispaced along
 * the path. They are stored in the array @pfl along with two defining
 * parameters.
 *
 * We will have
 *   pfl[0] = np, the number of points in the path
 *   pfl[1] = xi, the length of each increment
*/
#if 1
static
void qlrpfl(double pfl[], int klimx, int mdvarx, prop_type &prop, propv_type &propv)
{
	int np, j;
	double xl[2], q, za, zb;

	prop.d = pfl[0] * pfl[1];
	np = (int)pfl[0];

	// :44: determine horizons and dh from pfl, page 23
	hzns(pfl, prop);
	for (j = 0; j < 2; j++)
		xl[j] = mymin(15.0 * prop.h_g[j], 0.1 * prop.d_Lj[j]);

	xl[1] = prop.d - xl[1];
	prop.delta_h = dlthx(pfl, xl[0], xl[1]);

	if (prop.d_Lj[0] + prop.d_Lj[1] > 1.5 * prop.d) {
		// :45: Redo line-of-sight horizons, page 23

		/*
		 * If the path is line-of-sight, we still need to know where
		 * the horizons might have been, and so we turn to
		 * techniques used in area prediction mode.
		 */
		zlsq1(pfl, xl[0], xl[1], za, zb);
		prop.h_e[0] = prop.h_g[0] + FORTRAN_DIM(pfl[2], za);
		prop.h_e[1] = prop.h_g[1] + FORTRAN_DIM(pfl[np+2], zb);

		for (j = 0; j < 2; j++)
			prop.d_Lj[j] = sqrt(2.0 * prop.h_e[j] / prop.gamma_e) * exp(-0.07 * sqrt(prop.delta_h / mymax(prop.h_e[j], 5.0)));

		q = prop.d_Lj[0] + prop.d_Lj[1];

		if (q <= prop.d) {
			q = ((prop.d / q) * (prop.d / q));

			for (j = 0; j < 2; j++) {
				prop.h_e[j] *= q;
				prop.d_Lj[j] = sqrt(2.0 * prop.h_e[j] / prop.gamma_e) * exp(-0.07 * sqrt(prop.delta_h / mymax(prop.h_e[j], 5.0)));
			}
		}

		for (j = 0; j < 2; j++) {
			q = sqrt(2.0 * prop.h_e[j] / prop.gamma_e);
			prop.theta_ej[j] = (0.65 * prop.delta_h * (q / prop.d_Lj[j] - 1.0) - 2.0 * prop.h_e[j]) / q;
		}
	} else {
		// :46: Transhorizon effective heights, page 23
		zlsq1(pfl, xl[0], 0.9 * prop.d_Lj[0], za, q);
		zlsq1(pfl, prop.d - 0.9 * prop.d_Lj[1], xl[1], q, zb);
		prop.h_e[0] = prop.h_g[0] + FORTRAN_DIM(pfl[2], za);
		prop.h_e[1] = prop.h_g[1] + FORTRAN_DIM(pfl[np+2], zb);
	}

	prop.mdp = -1;
	propv.lvar = mymax(propv.lvar, 3);

	if (mdvarx >= 0) {
		propv.mdvar = mdvarx;
		propv.lvar = mymax(propv.lvar, 4);
	}

	if (klimx > 0) {
		propv.klim = klimx;
		propv.lvar = 5;
	}

	lrprop(0.0, prop);
}
#endif


//********************************************************
//* Point-To-Point Mode Calculations                     *
//********************************************************


#ifdef WITH_POINT_TO_POINT
#include <string.h>
static
void point_to_point(double elev[],
                    double tht_m,              // Transceiver above ground level
                    double rht_m,              // Receiver above groud level
                    double eps_dielect,        // Earth dielectric constant (rel. permittivity)
                    double sgm_conductivity,   // Earth conductivity (Siemens/m)
                    double eno,                // Atmospheric bending const, n-Units
                    double frq_mhz,
                    int radio_climate,         // 1..7
                    int pol,                   // 0 horizontal, 1 vertical
                    double conf,               // 0.01 .. .99, Fractions of situations
                    double rel,                // 0.01 .. .99, Fractions of time
                    double &dbloss,
                    char *strmode,
                    int &p_mode,				// propagation mode selector
                    double (&horizons)[2],			// horizon distances
                    int &errnum)
{
	// radio_climate: 1-Equatorial, 2-Continental Subtropical, 3-Maritime Tropical,
	//                4-Desert, 5-Continental Temperate, 6-Maritime Temperate, Over Land,
	//                7-Maritime Temperate, Over Sea
	// elev[]: [num points - 1], [delta dist(meters)], [height(meters) point 1], ..., [height(meters) point n]
	// errnum: 0- No Error.
	//         1- Warning: Some parameters are nearly out of range.
	//                     Results should be used with caution.
	//         2- Note: Default parameters have been substituted for impossible ones.
	//         3- Warning: A combination of parameters is out of range.
	//                     Results are probably invalid.
	//         Other-  Warning: Some parameters are out of range.
	//                          Results are probably invalid.

	prop_type   prop;
	propv_type  propv;

	double zsys = 0;
	double zc, zr;
	double q = eno;
	long ja, jb, i, np;
	double fs;

	prop.h_g[0] = tht_m;          // Tx height above ground level
	prop.h_g[1] = rht_m;          // Rx height above ground level
	propv.klim = radio_climate;
	prop.kwx = 0;                // no error yet
	propv.lvar = 5;              // initialize all constants
	prop.mdp = -1;               // point-to-point
	zc = qerfi(conf);
	zr = qerfi(rel);
	np = (long)elev[0];
#if 0
	double dkm = (elev[1] * elev[0]) / 1000.0;
	double xkm = elev[1] / 1000.0;
#endif

	ja = (long)(3.0 + 0.1 * elev[0]);
	jb = np - ja + 6;
	for (i = ja - 1; i < jb; ++i)
		zsys += elev[i];
	zsys /= (jb - ja + 1);

	propv.mdvar = 12;
	qlrps(frq_mhz, zsys, q, pol, eps_dielect, sgm_conductivity, prop);
	qlrpfl(elev, propv.klim, propv.mdvar, prop, propv);
	fs = 32.45 + 20.0 * log10(frq_mhz) + 20.0 * log10(prop.d / 1000.0);
	q = prop.d - prop.d_L;

	horizons[0] = 0.0;
	horizons[1] = 0.0;
	if (int(q) < 0.0) {
		strcpy(strmode, "Line-Of-Sight Mode");
		p_mode = 0;
	} else {
		if (int(q) == 0.0) {
			strcpy(strmode, "Single Horizon");
			horizons[0] = prop.d_Lj[0];
			p_mode = 1;
		}

		else {
			if (int(q) > 0.0) {
				strcpy(strmode, "Double Horizon");
				horizons[0] = prop.d_Lj[0];
				horizons[1] = prop.d_Lj[1];
				p_mode = 1;
			}
		}

		if (prop.d <= prop.d_Ls || prop.d <= prop.dx) {
			strcat(strmode, ", Diffraction Dominant");
			p_mode = 1;
		}

		else {
			if (prop.d > prop.dx) {
				strcat(strmode, ", Troposcatter Dominant");
				p_mode = 2;
			}
		}
	}

	dbloss = avar(zr, 0.0, zc, prop, propv) + fs;
	errnum = prop.kwx;
}
#endif


#ifdef WITH_POINT_TO_POINT_MDH
static
void point_to_pointMDH(double elev[], double tht_m, double rht_m, double eps_dielect, double sgm_conductivity, double eno, double frq_mhz, int radio_climate, int pol, double timepct, double locpct, double confpct, double &dbloss, int &propmode, double &deltaH, int &errnum)
{
	// pol: 0-Horizontal, 1-Vertical
	// radio_climate: 1-Equatorial, 2-Continental Subtropical, 3-Maritime Tropical,
	//                4-Desert, 5-Continental Temperate, 6-Maritime Temperate, Over Land,
	//                7-Maritime Temperate, Over Sea
	// timepct, locpct, confpct: .01 to .99
	// elev[]: [num points - 1], [delta dist(meters)], [height(meters) point 1], ..., [height(meters) point n]
	// propmode:  Value   Mode
	//             -1     mode is undefined
	//              0     Line of Sight
	//              5     Single Horizon, Diffraction
	//              6     Single Horizon, Troposcatter
	//              9     Double Horizon, Diffraction
	//             10     Double Horizon, Troposcatter
	// errnum: 0- No Error.
	//         1- Warning: Some parameters are nearly out of range.
	//                     Results should be used with caution.
	//         2- Note: Default parameters have been substituted for impossible ones.
	//         3- Warning: A combination of parameters is out of range.
	//                     Results are probably invalid.
	//         Other-  Warning: Some parameters are out of range.
	//                          Results are probably invalid.

	prop_type   prop;
	propv_type  propv;
	propa_type  propa;
	double zsys = 0;
	double ztime, zloc, zconf;
	double q = eno;
	long ja, jb, i, np;
	double dkm, xkm;
	double fs;

	propmode = -1; // mode is undefined
	prop.h_g[0] = tht_m;
	prop.h_g[1] = rht_m;
	propv.klim = radio_climate;
	prop.kwx = 0;
	propv.lvar = 5;
	prop.mdp = -1;
	ztime = qerfi(timepct);
	zloc = qerfi(locpct);
	zconf = qerfi(confpct);

	np = (long)elev[0];
	dkm = (elev[1] * elev[0]) / 1000.0;
	xkm = elev[1] / 1000.0;

	ja = (long)(3.0 + 0.1 * elev[0]);
	jb = np - ja + 6;
	for (i = ja - 1; i < jb; ++i)
		zsys += elev[i];
	zsys /= (jb - ja + 1);

	propv.mdvar = 12;
	qlrps(frq_mhz, zsys, q, pol, eps_dielect, sgm_conductivity, prop);
	qlrpfl(elev, propv.klim, propv.mdvar, prop, propa, propv);
	fs = 32.45 + 20.0 * log10(frq_mhz) + 20.0 * log10(prop.d / 1000.0);
	deltaH = prop.delta_h;
	q = prop.d - prop.d_L;

	if (int(q) < 0.0) {
		propmode = 0; // Line-Of-Sight Mode
	} else {
		if (int(q) == 0.0)
			propmode = 4; // Single Horizon
		else
			if (int(q) > 0.0)
				propmode = 8; // Double Horizon

		if (prop.d <= prop.d_Ls || prop.d <= prop.dx)
			propmode += 1; // Diffraction Dominant

		else
		if (prop.d > prop.dx)
			propmode += 2; // Troposcatter Dominant
	}

	dbloss = avar(ztime, zloc, zconf, prop, propv) + fs;  //avar(time,location,confidence)
	errnum = prop.kwx;
}
#endif


#ifdef WITH_POINT_TO_POINT_DH
static
void point_to_pointDH(double elev[], double tht_m, double rht_m, double eps_dielect, double sgm_conductivity, double eno, double frq_mhz, int radio_climate, int pol, double conf, double rel, double &dbloss, double &deltaH, int &errnum)
{
	// pol: 0-Horizontal, 1-Vertical
	// radio_climate: 1-Equatorial, 2-Continental Subtropical, 3-Maritime Tropical,
	//                4-Desert, 5-Continental Temperate, 6-Maritime Temperate, Over Land,
	//                7-Maritime Temperate, Over Sea
	// conf, rel: .01 to .99
	// elev[]: [num points - 1], [delta dist(meters)], [height(meters) point 1], ..., [height(meters) point n]
	// errnum: 0- No Error.
	//         1- Warning: Some parameters are nearly out of range.
	//                     Results should be used with caution.
	//         2- Note: Default parameters have been substituted for impossible ones.
	//         3- Warning: A combination of parameters is out of range.
	//                     Results are probably invalid.
	//         Other-  Warning: Some parameters are out of range.
	//                          Results are probably invalid.

	char strmode[100];
	prop_type   prop;
	propv_type  propv;
	propa_type  propa;
	double zsys = 0;
	double zc, zr;
	double q = eno;
	long ja, jb, i, np;
	double dkm, xkm;
	double fs;

	prop.h_g[0] = tht_m;
	prop.h_g[1] = rht_m;
	propv.klim = radio_climate;
	prop.kwx = 0;
	propv.lvar = 5;
	prop.mdp = -1;
	zc = qerfi(conf);
	zr = qerfi(rel);
	np = (long)elev[0];
	dkm = (elev[1] * elev[0]) / 1000.0;
	xkm = elev[1] / 1000.0;

	ja = (long)(3.0 + 0.1 * elev[0]);
	jb = np - ja + 6;
	for (i = ja - 1; i < jb; ++i)
		zsys += elev[i];
	zsys /= (jb - ja + 1);

	propv.mdvar = 12;
	qlrps(frq_mhz, zsys, q, pol, eps_dielect, sgm_conductivity, prop);
	qlrpfl(elev, propv.klim, propv.mdvar, prop, propa, propv);
	fs = 32.45 + 20.0 * log10(frq_mhz) + 20.0 * log10(prop.d / 1000.0);
	deltaH = prop.delta_h;
	q = prop.d - prop.d_L;

	if (int(q) < 0.0)
		strcpy(strmode, "Line-Of-Sight Mode");
	else {
		if (int(q) == 0.0)
			strcpy(strmode, "Single Horizon");

		else
		if (int(q) > 0.0)
			strcpy(strmode, "Double Horizon");

		if (prop.d <= prop.d_Ls || prop.d <= prop.dx)
			strcat(strmode, ", Diffraction Dominant");

		else
		if (prop.d > prop.dx)
			strcat(strmode, ", Troposcatter Dominant");
	}

	dbloss = avar(zr, 0.0, zc, prop, propv) + fs; //avar(time,location,confidence)
	errnum = prop.kwx;
}
#endif



//********************************************************
//* Area Mode Calculations                               *
//********************************************************


#ifdef WITH_AREA
static
void area(long ModVar, double deltaH, double tht_m, double rht_m, double dist_km, int TSiteCriteria, int RSiteCriteria, double eps_dielect, double sgm_conductivity, double eno, double frq_mhz, int radio_climate, int pol, double pctTime, double pctLoc, double pctConf, double &dbloss, int &errnum)
{
	// pol: 0-Horizontal, 1-Vertical
	// TSiteCriteria, RSiteCriteria:
	//		   0 - random, 1 - careful, 2 - very careful
	// radio_climate: 1-Equatorial, 2-Continental Subtropical, 3-Maritime Tropical,
	//                4-Desert, 5-Continental Temperate, 6-Maritime Temperate, Over Land,
	//                7-Maritime Temperate, Over Sea
	// ModVar: 0 - Single: pctConf is "Time/Situation/Location", pctTime, pctLoc not used
	//         1 - Individual: pctTime is "Situation/Location", pctConf is "Confidence", pctLoc not used
	//         2 - Mobile: pctTime is "Time/Locations (Reliability)", pctConf is "Confidence", pctLoc not used
	//         3 - Broadcast: pctTime is "Time", pctLoc is "Location", pctConf is "Confidence"
	// pctTime, pctLoc, pctConf: .01 to .99
	// errnum: 0- No Error.
	//         1- Warning: Some parameters are nearly out of range.
	//                     Results should be used with caution.
	//         2- Note: Default parameters have been substituted for impossible ones.
	//         3- Warning: A combination of parameters is out of range.
	//                     Results are probably invalid.
	//         Other-  Warning: Some parameters are out of range.
	//                          Results are probably invalid.
	// NOTE: strmode is not used at this time.

	prop_type prop;
	propv_type propv;
	propa_type propa;
	double zt, zl, zc, xlb;
	double fs;
	long ivar;
	double eps, sgm;
	long ipol;
	int kst[2];

	kst[0] = (int)TSiteCriteria;
	kst[1] = (int)RSiteCriteria;
	zt = qerfi(pctTime);
	zl = qerfi(pctLoc);
	zc = qerfi(pctConf);
	eps = eps_dielect;
	sgm = sgm_conductivity;
	prop.delta_h = deltaH;
	prop.h_g[0] = tht_m;
	prop.h_g[1] = rht_m;
	propv.klim = (long)radio_climate;
	prop.N_s = eno;
	prop.kwx = 0;
	ivar = (long)ModVar;
	ipol = (long)pol;
	qlrps(frq_mhz, 0.0, eno, ipol, eps, sgm, prop);
	qlra(kst, propv.klim, ivar, prop, propv);

	if (propv.lvar < 1)
		propv.lvar = 1;

	lrprop(dist_km * 1000.0, prop, propa);
	fs = 32.45 + 20.0 * log10(frq_mhz) + 20.0 * log10(prop.d / 1000.0);
	xlb = fs + avar(zt, zl, zc, prop, propv);
	dbloss = xlb;

	if (prop.kwx == 0)
		errnum = 0;
	else
		errnum = prop.kwx;
}
#endif

}
