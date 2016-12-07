/*---------------------------------------------------------------------
*
* Copyright © 2015  Minsi Chen
* E-mail: m.chen@derby.ac.uk
*
* The source is written for the Graphics I and II modules. You are free
* to use and extend the functionality. The code provided here is functional
* however the author does not guarantee its performance.
---------------------------------------------------------------------*/
#include <stdlib.h>
#include "Material.h"
#include "ImageIO.h"

Material::Material()
{
	SetDefaultMaterial();
}


Material::~Material()
{
	if (mDiffuse_texture) delete mDiffuse_texture;
}

void Material::SetDefaultMaterial()
{
	//default material
	//black ambient colour
	//white diffuse and specular
	//spec power is 10

	SetAmbientColour(0.0, 0.0, 0.0);
	SetDiffuseColour(1.0, 1.0, 1.0);
	SetSpecularColour(1.0, 1.0, 1.0);
	SetSpecPower(10.0);
	mDiffuse_texture = NULL;
	mNormal_texture = NULL;
	mCastShadow = true;
	mReflectivity = 0;
	mRefractiveIndex = 0;
}

void Material::SetAmbientColour(float r, float g, float b)
{
	mAmbient.SetVector(r, g, b);
}

void Material::SetDiffuseColour(float r, float g, float b)
{
	mDiffuse.SetVector(r, g, b);
}

void Material::SetSpecularColour(float r, float g, float b)
{
	mSpecular.SetVector(r, g, b);
}

void Material::SetSpecPower(double spow)
{
	mSpecpower = spow;
}
Colour Texture::GetBiLinearFilteredColour(double u, double v) {
	if (mImage == NULL) {
		//ether the image failed to load or the image was never set 
		//so we return black
		return Colour(0, 0, 0);
	}
	// fetch a bilinearly filtered texel
	float Du = (u)* mHeight - 0.5;
	float Dv = (v)* mHeight - 0.5;//hmm
	int x = floorf(Du);
	int y = floorf(Dv);
	float Uratio = Du - x;
	float Vratio = Dv - y;
	float Uopp = 1 - Uratio;
	float Vopp = 1 - Vratio;
	Colour finalcol = Colour((GetTexelColour(x, y)*Uopp + GetTexelColour(x + 1, y)*Uratio)*Vopp + (GetTexelColour(x, y + 1)*Vopp + GetTexelColour(x + 1, y + 1)*Vratio)*Vratio);
	return finalcol;
}
Colour Texture::GetTexelColour(double u, double v)
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
		texel_loc -= msize;//wrap
		if (texel_loc > msize - 1) {
			texel_loc -= msize;//wrap
			if (texel_loc > msize - 1) {
				texel_loc -= msize;//wrap
			}
		}

	}
	if (texel_loc > msize - 1) {
		texel_loc = msize;//clamps the texture
	}
	if (texel_loc < 0) {
		texel_loc = 0;//if the texture ptr is STILL out of range
	}
	unsigned char* comp = mImage + texel_loc;
	//.bmp is not RGB
	//its BGR for some reason
	colour[0] = *(comp + 2) / 255.0f;
	colour[1] = *(comp + 1) / 255.0f;
	colour[2] = *(comp) / 255.0f;

	return colour;
}



void Texture::LoadTextureFromFile(char * filename, bool tga)
{
	if (tga == true)
	{		
		//DID SOMEONE forget to implement the TGA loader!?

		/*int sizey, sizex, bpp = 0;
		if (ImageIO::LoadTGA(filename, &mImage, &sizey, &sizex, &bpp, &mChannels) == E_IMAGEIO_ERROR) {
			printf("Read Error\n");
		}*/
	}
	else
	{
		//open file
		std::fstream hFile(filename, std::ios::in | std::ios::binary);
		if (!hFile.is_open()) {
			std::cout << "Error: File " << filename << " Not Found." << std::endl;
			mImage = NULL;
			return;
		}
		//seek to the EOF
		hFile.seekg(0, std::ios::end);
		//get the positon in the stream
		//we now know the length of file!
		std::size_t Length = hFile.tellg();
		//seek to the begining again
		hFile.seekg(0, std::ios::beg);
		std::vector<std::uint8_t> FileInfo(Length);
		hFile.read(reinterpret_cast<char*>(FileInfo.data()), 54);
		//read file info bits to ensure the BMP is one we can process
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

		mChannels = FileInfo[28]/8;
		mWidth = FileInfo[18] + (FileInfo[19] << 8);
		mHeight = FileInfo[22] + (FileInfo[23] << 8);
		//calulate the offset to pixels so we dont read the fileinfo as pixels
		std::uint32_t PixelsOffset = FileInfo[10] + (FileInfo[11] << 8);
		std::uint32_t size = ((mWidth * FileInfo[28] + 31) / 32) * 4 * mHeight;
		Pixels.resize(size);
		msize = size;
		//start reading from the start of the pixel infromation 
		hFile.seekg(PixelsOffset, std::ios::beg);
		//reinterpret_cast ensures that we get our data back (ISH i mean its C/C++ so anything could happen)
		hFile.read(reinterpret_cast<char*>(Pixels.data()), size);
		hFile.close();
		mImage = new unsigned char[size];
		//read them pixels
		for (int i = 0; i < Pixels.size(); i++) {
			*(mImage + i) = Pixels[i];
		}
	}


}
