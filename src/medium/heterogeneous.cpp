#include "heterogeneous.h"


/* ****************************** blackbody ******************************** */

/* Calculate color in range 800..12000 using an approximation
 * a/x+bx+c for R and G and ((at + b)t + c)t + d) for B
 * Max absolute error for RGB is (0.00095, 0.00077, 0.00057),
 * which is enough to get the same 8 bit/channel color.
 */
static const float blackbody_table_r[6][3] = {
	{2.52432244e+03f, -1.06185848e-03f, 3.11067539e+00f},
	{3.37763626e+03f, -4.34581697e-04f, 1.64843306e+00f},
	{4.10671449e+03f, -8.61949938e-05f, 6.41423749e-01f},
	{4.66849800e+03f, 2.85655028e-05f, 1.29075375e-01f},
	{4.60124770e+03f, 2.89727618e-05f, 1.48001316e-01f},
	{3.78765709e+03f, 9.36026367e-06f, 3.98995841e-01f},
};

static const float blackbody_table_g[6][3] = {
	{-7.50343014e+02f, 3.15679613e-04f, 4.73464526e-01f},
	{-1.00402363e+03f, 1.29189794e-04f, 9.08181524e-01f},
	{-1.22075471e+03f, 2.56245413e-05f, 1.20753416e+00f},
	{-1.42546105e+03f, -4.01730887e-05f, 1.44002695e+00f},
	{-1.18134453e+03f, -2.18913373e-05f, 1.30656109e+00f},
	{-5.00279505e+02f, -4.59745390e-06f, 1.09090465e+00f},
};

static const float blackbody_table_b[6][4] = {
	{0.0f, 0.0f, 0.0f, 0.0f},
	{0.0f, 0.0f, 0.0f, 0.0f},
	{0.0f, 0.0f, 0.0f, 0.0f},
	{-2.02524603e-11f, 1.79435860e-07f, -2.60561875e-04f, -1.41761141e-02f},
	{-2.22463426e-13f, -1.55078698e-08f, 3.81675160e-04f, -7.30646033e-01f},
	{6.72595954e-13f, -2.73059993e-08f, 4.24068546e-04f, -7.52204323e-01f},
};

static void blackbody_temperature_to_rgb(float rgb[3], float t)
{
	if (t < 100) {
		rgb[0] = rgb[1] = rgb[2] = 0.f;
		return;
	}
	if (t >= 12000.0f) {
		rgb[0] = 0.826270103f;
		rgb[1] = 0.994478524f;
		rgb[2] = 1.56626022f;
	}
	else if (t < 965.0f) {
		rgb[0] = 4.70366907f;
		rgb[1] = 0.0f;
		rgb[2] = 0.0f;
	}
	else {
		int i = (t >= 6365.0f) ?
			5 :
			(t >= 3315.0f) ? 4 :
			(t >= 1902.0f) ? 3 : (t >= 1449.0f) ? 2 : (t >= 1167.0f) ? 1 : 0;

		const float* r = blackbody_table_r[i];
		const float* g = blackbody_table_g[i];
		const float* b = blackbody_table_b[i];

		const float t_inv = 1.0f / t;
		rgb[0] = r[0] * t_inv + r[1] * t + r[2];
		rgb[1] = g[0] * t_inv + g[1] * t + g[2];
		rgb[2] = ((b[0] * t + b[1]) * t + b[2]) * t + b[3];
	}
}

/*
void blackbody_temperature_to_rgb_table(float* r_table, int width, float min, float max)
{
	for (int i = 0; i < width; i++) {
		float temperature = min + (max - min) / (float)width * (float)i;

		float rgb[3];
		blackbody_temperature_to_rgb(rgb, temperature);

		copy_v3_v3(&r_table[i * 4], rgb);
		r_table[i * 4 + 3] = 0.0f;
	}
}
*/

HeterogeneousMedium::HeterogeneousMedium(
	const std::shared_ptr<PhaseFunction>& pf,
	const std::string& filename,
	const bool& lefthand,
	const std::string& densityName,
	const Spectrum& albedo,
	const float& scale,
	const std::string& temperatureName,
	const float& temperatureScale)
	: Medium(pf), m_lefthand(lefthand), m_albedo(albedo), m_scale(scale), m_temperatureScale(temperatureScale),
	m_densitySampler({}), m_temperatureSampler({})
{
	openvdb::initialize();
	openvdb::io::File file(GetFileResolver()->string() + "/" + filename);
	std::cout << "Loading " << filename << std::endl;

	file.open();
	openvdb::GridBase::Ptr densityBaseGrid;
	densityBaseGrid = file.readGrid(densityName);
	m_densityGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(densityBaseGrid);
	m_densitySampler = VDBFloatSampler(*m_densityGrid);

	openvdb::GridBase::Ptr temperatureBaseGrid;
	temperatureBaseGrid = file.readGrid(temperatureName);
	std::cout << temperatureName << std::endl;
	m_temperatureGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(temperatureBaseGrid);
	m_temperatureSampler = VDBFloatSampler(*m_temperatureGrid);
	file.close();

	m_densityGrid->evalMinMax(m_minDensity, m_maxDensity);
	m_minDensity = 0.f;
	m_invMaxDensity = m_maxDensity == 0 ? 0 : 1.f / m_maxDensity;

	//m_temperatureScale = 5100;
	std::cout << m_minDensity << ' ' << m_maxDensity << std::endl;
}

Spectrum HeterogeneousMedium::Sample(const Ray& ray, MediumRecord& mediumRec, Sampler& sampler) const
{
	float d = ray.tMax;
	LOG_IF(FATAL, d == std::numeric_limits<float>::infinity()) << "The estimated ray is infinity.";
	float t = 0;
	float Tr = 1.f, pdf;
	while (true) {
		float step = -std::log(1 - sampler.Next1D()) / (m_maxDensity * m_scale);
		t += step;
		// Outside the medium
		if (t >= d) {
			mediumRec.m_internal = false;
			mediumRec.m_Le = Spectrum(0.f);
			mediumRec.m_p = ray(d);
			return 1.f;
		}
		// Inside the medium
		Float3 p = ray(t);
		float sigmaT = Density(p);
		float s = sampler.Next1D();
		if (s < m_albedo.Average() * sigmaT / m_maxDensity) {
			mediumRec.m_internal = true;
			mediumRec.m_Le = Spectrum(0.f);
			mediumRec.m_p = ray(t);
			return m_albedo;
		}
		else if (s < sigmaT / m_maxDensity) {
			Spectrum Le;
			float temperature = Temperature(p) * m_temperatureScale;
			blackbody_temperature_to_rgb(&Le.r, temperature);			
			if (temperature < 1100) {
				Le = Spectrum(0.f);
			}
			mediumRec.m_Le = Le * 3;
			//mediumRec.m_Le = Spectrum(0.f);
			return 0.f;
		}
		
		// Get sigma_a, sigma_s(albedo is single channel)
		
		/*
		float s = sampler.Next1D();		
		if (s < Density(ray(t)) / m_maxDensity) {
			mediumRec.m_internal = true;
			mediumRec.m_p = ray(t);
			return m_albedo;
		}
		
		

		
		float sigma_t_bar = m_maxDensity * m_scale;
		float sigma_t = Density(ray(t)) * m_scale;
		float sigma_s = sigma_t * m_albedo.r;
		float sigma_a = sigma_t * std::max(0.f, 1 - m_albedo.r);
		float sigma_n = sigma_t_bar - sigma_t;

		Tr *= std::exp(m_maxDensity * m_scale * (-step));
		pdf *= std::exp(m_maxDensity * m_scale * (-step));
		// Sample particle's kind
		//int mod = SampleDiscrete({ sigma_s, sigma_a, sigma_n }, sampler.Next1D());
		int mod;
		s = sampler.Next1D();
		if (s <= sigma_s / sigma_t_bar) mod = 0;
		else if (s >= 1 - (sigma_n / sigma_t_bar)) mod = 2;
		else mod = 1;
		//if (mod >= 1) mod = 2;		

		//LOG_IF(FATAL, mod == 1) << "No absorption for now";

		if (mod == 0) {
			// Scatter
			//Tr *= sigma_s;
			//pdf *= sigma_s;
			// Hit authentic particle
			mediumRec.m_p = ray(t);
			mediumRec.m_t = t;
			mediumRec.m_pdf = pdf;
			mediumRec.m_internal = true;
			mediumRec.m_Le = Spectrum(0.f);
			return m_albedo;
			float ret = pdf == 0 ? 0.f : Tr / pdf;
			if (std::isnan(ret)) {
				std::cout << Tr << ' ' << pdf << std::endl;
			}
			return pdf == 0 ? 0.f : Tr / pdf;
		}
		else if (mod == 1) {
			// Absorb
			//Tr *= sigma_a;
			//pdf *= sigma_a;
			// Hit emitter


			Spectrum Le;
			float temperature = Temperature(ray(t)) * m_temperatureScale;
			blackbody_temperature_to_rgb(&Le.r, temperature);
			//std::cout << temperature << ' ' << Le << std::endl;
			mediumRec.m_Le = Tr * Le;
			//std::cout << mediumRec.m_Le.IsBlack() << std::endl;
			return 0.f;
		}
		else if (mod == 2) {
			// Null
			//Tr *= (sigma_t_bar - sigma_s - sigma_a);
			//pdf *= (sigma_t_bar - sigma_s - sigma_a);
			// Hit fictional particle
		}
		*/
	}
	//LOG_IF(FATAL, true) << "Fuck";
	return Spectrum(1.f);
}

Spectrum HeterogeneousMedium::Transmittance(const Ray& ray, Sampler& sampler) const
{
	float d = ray.tMax;
	LOG_IF(FATAL, d == std::numeric_limits<float>::infinity()) << "The estimated ray is infinity.";
	float t = 0;
	float Tr = 1;

	while (true) {
		float s = sampler.Next1D();
		float step = -std::log(1 - s) / (m_maxDensity * m_scale);
		t += step;

		if (t >= d) break;

		float sigma_t_bar = m_maxDensity * m_scale;
		float sigma_t = Density(ray(t)) * m_scale;
		Tr *= (1 - sigma_t / sigma_t_bar);

		float rrThreshold = .1;
		if (Tr < rrThreshold) {
			float q = std::max((float).05, 1 - Tr);
			if (sampler.Next1D() < q) {
				return Spectrum(0.);
			}
			Tr /= 1 - q;
		}
	}

	return Tr;
}

float HeterogeneousMedium::Density(const Float3& _pWorld) const
{
	openvdb::Vec3d pWorld;
	if (m_lefthand) {
		pWorld = openvdb::Vec3d(_pWorld.x, _pWorld.z, -_pWorld.y);
	}
	else {
		pWorld = openvdb::Vec3d(_pWorld.x, _pWorld.y, _pWorld.z);
	}
	pWorld = pWorld * 20;
	float ret = m_densitySampler.wsSample(pWorld);
	return ret;
}

float HeterogeneousMedium::Temperature(const Float3& _pWorld) const
{
	openvdb::Vec3d pWorld;
	if (m_lefthand) {
		pWorld = openvdb::Vec3d(_pWorld.x, _pWorld.z, -_pWorld.y);
	}
	else {
		pWorld = openvdb::Vec3d(_pWorld.x, _pWorld.y, _pWorld.z);
	}
	pWorld = pWorld * 20;
	float ret = m_temperatureSampler.wsSample(pWorld);
	return ret;
}
