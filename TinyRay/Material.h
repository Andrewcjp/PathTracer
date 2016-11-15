/*---------------------------------------------------------------------
*
* Copyright © 2015  Minsi Chen
* E-mail: m.chen@derby.ac.uk
*
* The source is written for the Graphics I and II modules. You are free
* to use and extend the functionality. The code provided here is functional
* however the author does not guarantee its performance.
---------------------------------------------------------------------*/
#pragma once

#include <stdlib.h>

#include "Vector3.h"
#include <ios>
#include <iostream>
#include <fstream> 
#include <vector>

typedef Vector3 Colour;

//class representing a texture in TinyRay
class Texture
{
public:
	enum TEXUNIT
	{
		TEXUNIT_DIFFUSE = 0,
		TEXUNIT_NORMAL = 1
	};
private:
	std::vector<std::uint8_t> Pixels;
public:
	unsigned int	mWidth;			//width of the image
	unsigned int    mHeight;		//height of the image
	unsigned int    mChannels = 3;		//number of channels in the image either 3 or 4, i.e. RGB or RGBA
	unsigned char*	mImage;			//image data
	int msize;
	Texture()
	{
		mImage = NULL;
	}
	Texture(char* filename) {
		mImage = NULL;
		LoadTextureFromFile(filename);
	}

	~Texture()
	{
		if (mImage) delete[] mImage;
	}

	Colour GetTexelColour(double u, double v)
	{
		if (mImage == NULL) {
			//ether the image failed to load or the image was never set 
			//so we return black
			return Colour(0, 0, 0);
		}
		Colour colour;
		u = abs(u);
		v = abs(v);

		int dx = u*mWidth;
		int dy = v*mHeight;
		int texel_stride = mChannels;
		int row_stride = mWidth;
		int texel_loc = (dx*texel_stride) + dy*row_stride*texel_stride;
		//todo: this might be unsafe! shocker IT IS!
		if (texel_loc > msize - 1) {
			texel_loc -= msize;//clamp
			if (texel_loc > msize - 1) {
				texel_loc -= msize;//clamp
			}
		}

		unsigned char* comp = mImage + texel_loc;
		//bmp not rgb
		//todo remove this
		//bgr
		colour[0] = *(comp + 2) / 255.0f;
		colour[1] = *(comp + 1) / 255.0f;
		colour[2] = *(comp) / 255.0f;

		return colour;
	}
	//todo: covert to format
	void LoadTextureFromFile(char* filename) {

		//open file
		std::fstream hFile(filename, std::ios::in | std::ios::binary);
		if (!hFile.is_open()) {
			std::cout << "Error: File " << filename << " Not Found." << std::endl;
			mImage = NULL;
			return;
		}

		hFile.seekg(0, std::ios::end);
		std::size_t Length = hFile.tellg();
		hFile.seekg(0, std::ios::beg);
		std::vector<std::uint8_t> FileInfo(Length);
		hFile.read(reinterpret_cast<char*>(FileInfo.data()), 54);//wtf?

		if (FileInfo[0] != 'B' && FileInfo[1] != 'M')
		{
			hFile.close();
			std::cout << "Error: Invalid File Format. Bitmap Required." << std::endl;
			mImage = NULL;
			return;
		}

		if (FileInfo[28] != 24 && FileInfo[28] != 32)
		{
			hFile.close();
			std::cout << "Error: Invalid File Format. 24 or 32 bit Image Required." << std::endl;
			mImage = NULL;
			return;
		}
		//todo: support 32 bit with alpha
		//mChannels = FileInfo[28];
		mChannels = 3;
		mWidth = FileInfo[18] + (FileInfo[19] << 8);
		mHeight = FileInfo[22] + (FileInfo[23] << 8);

		std::uint32_t PixelsOffset = FileInfo[10] + (FileInfo[11] << 8);
		std::uint32_t size = ((mWidth * FileInfo[28] + 31) / 32) * 4 * mHeight;
		Pixels.resize(size);
		msize = size;
		hFile.seekg(PixelsOffset, std::ios::beg);
		hFile.read(reinterpret_cast<char*>(Pixels.data()), size);
		hFile.close();
		mImage = new unsigned char[size];
		for (int i = 0; i < Pixels.size(); i++) {
			*(mImage + i) = Pixels[i];
		}
	}
};

//Class representing a material in TinyRay
class Material
{
private:
	Colour mAmbient;					//Ambient colour of the material
	Colour mDiffuse;					//Diffuse colour of the material
	Colour mSpecular;					//Specular colour of the material
	double mSpecpower;					//Specular power of the material
	Texture* mDiffuse_texture;			//Colour (diffuse) texture of the material for texture mapped primitives
	Texture* mNormal_texture;			//Normal texture of the material for textured mapped privmites with a normal map
	bool mCastShadow;					//boolean indicating if a material can cast shadow
	double mReflectivity;
	double mRefractiveIndex;

public:

	Material();
	~Material();

	void SetDefaultMaterial();
	void SetAmbientColour(float r, float g, float b);
	void SetDiffuseColour(float r, float g, float b);
	void SetSpecularColour(float r, float g, float b);
	void SetSpecPower(double spow);
	inline void SetDiffuseTexture(Texture* tex) {
		mDiffuse_texture = tex;
	}
	inline void SetNormalTexture(Texture* tex) {
		mNormal_texture = tex;
	}
	inline Texture* GetDiffuseTexture() {
		return mDiffuse_texture;
	}
	inline Texture* GetNormalTexture() {
		return mDiffuse_texture;
	}
	inline void SetCastShadow(bool castShadow)
	{
		mCastShadow = castShadow;
	}
	inline void SetReflectivity(double amt) {
		mReflectivity = amt;
	}
	inline void SetRefractivity(double amt) {
		mRefractiveIndex = amt;
	}
	inline double GetReflectivity() {
		return	mReflectivity;
	}
	inline double GetRefractivity() {
		return mRefractiveIndex;
	}
	inline Colour& GetAmbientColour()
	{
		return mAmbient;
	}

	inline Colour& GetDiffuseColour()
	{

		return mDiffuse;
	}

	inline Colour& GetSpecularColour()
	{
		return mSpecular;
	}

	inline double GetSpecPower()
	{
		return mSpecpower;
	}

	inline bool CastShadow()
	{
		return mCastShadow;
	}

	inline bool HasDiffuseTexture()
	{
		return mDiffuse_texture != NULL;
	}

	inline bool HasNormalTexture()
	{
		return mNormal_texture != NULL;
	}
};

