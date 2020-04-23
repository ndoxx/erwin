#!/usr/bin/python3
import sys
import numpy as np
from PIL import Image, ImageOps

# Horrendously slow CPU program to generate a BRDF integration map
# Yeah, I'm going for a GPU implementation soon

def normalize(V):
	return V / np.sqrt(np.sum(V**2))

def RadicalInverse_VdC(bits):
	 bits = (bits << 16) | (bits >> 16)
	 bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1)
	 bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2)
	 bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4)
	 bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8)
	 return np.float(bits * 2.3283064365386963e-10) # 0x100000000

def Hammersley(ii, N):
	return np.array([np.float(ii)/np.float(N), RadicalInverse_VdC(ii)])

def ImportanceSampleGGX(Xi, N, roughness):
	a = np.float(roughness*roughness)
	
	phi = np.float(2.0 * np.pi * Xi[0])
	cosTheta = np.sqrt((1.0 - Xi[1]) / (1.0 + (a*a - 1.0) * Xi[1]))
	sinTheta = np.sqrt(1.0 - cosTheta*cosTheta);
	
	# from spherical coordinates to cartesian coordinates - halfway vector
	H = np.array([np.cos(phi) * sinTheta, np.sin(phi) * sinTheta, cosTheta])
	
	# from tangent-space H vector to world-space sample vector
	up        = np.array([0.0, 0.0, 1.0]) if np.abs(N[2]) < 0.999 else np.array([1.0, 0.0, 0.0])
	tangent   = normalize(np.cross(up, N))
	bitangent = np.cross(N, tangent)
	
	sampleVec = tangent * H[0] + bitangent * H[1] + N * H[2]
	return normalize(sampleVec)

def GeometrySchlickGGX(NdotV, roughness):
	# note that we use a different k for IBL
	a = roughness
	k = (a * a) / 2.0
	nom   = NdotV
	denom = NdotV * (1.0 - k) + k
	return np.float(nom / denom)

def GeometrySmith(N, V, L, roughness):
	NdotV = np.maximum(np.dot(N, V), 0.0)
	NdotL = np.maximum(np.dot(N, L), 0.0)
	ggx2 = GeometrySchlickGGX(NdotV, roughness)
	ggx1 = GeometrySchlickGGX(NdotL, roughness)
	return np.float(ggx1 * ggx2)

def IntegrateBRDF(NdotV, roughness):
	V = np.array([np.sqrt(1.0 - NdotV*NdotV), 0.0, NdotV])
	A = np.float(0.0)
	B = np.float(0.0) 
	N = np.array([0.0, 0.0, 1.0])
    
	SAMPLE_COUNT = 10;
	for ii in range(SAMPLE_COUNT):
		# generates a sample vector that's biased towards the
		# preferred alignment direction (importance sampling).
		Xi = Hammersley(ii, SAMPLE_COUNT)
		H  = ImportanceSampleGGX(Xi, N, roughness)
		L  = normalize(2.0 * np.dot(V, H) * H - V)

		NdotL = np.float(np.maximum(L[2], 0.0))
		NdotH = np.float(np.maximum(H[2], 0.0))
		VdotH = np.float(np.maximum(np.dot(V, H), 0.0))

		if NdotL > 0.0:
			G     = GeometrySmith(N, V, L, roughness)
			G_Vis = (G * VdotH) / np.maximum(NdotH * NdotV, 0.00001)
			Fc    = np.power(1.0 - VdotH, 5.0)
			A += (1.0 - Fc) * G_Vis
			B += Fc * G_Vis

	A /= np.float(SAMPLE_COUNT)
	B /= np.float(SAMPLE_COUNT)
	return np.array([A, B])

def main(argv):
	img_size = 512
	brdf_integration_map = np.zeros(shape=(img_size,img_size,3), dtype=np.uint8)

	for xx in range(img_size):
		u = np.float(xx/img_size)
		for yy in range(img_size):
			v = np.float(yy/img_size)
			brdf = IntegrateBRDF(u,v)
			brdf_integration_map[yy,xx,0] = np.uint8(255*brdf[0])
			brdf_integration_map[yy,xx,1] = np.uint8(255*brdf[1])
			brdf_integration_map[yy,xx,2] = 0
		print("Progress: " + str(u*100) + "%")

	im = Image.fromarray(brdf_integration_map)
	im_flip = ImageOps.flip(im)
	im_flip.save('ibl_brdf_integration.png')


if __name__ == '__main__':
    main(sys.argv[1:])