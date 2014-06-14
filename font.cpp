#include "font.h"
#include <chthon2/pixmap.h>
#include <chthon2/util.h>
#include <SDL2/SDL.h>
#include <iostream>
using namespace Chthon;

namespace Sprite {
#include "res/font.xpm"
SDL_Texture * load(SDL_Renderer * renderer, const char ** xpm, int size);
}

SDL_Texture * Sprite::load(SDL_Renderer * renderer, const char ** xpm, int size)
{
	SDL_Texture * result = 0;
	try {
		std::vector<std::string> xpm_lines(xpm, xpm + size);
		Chthon::Pixmap pixmap;
		pixmap.load(xpm_lines);
		SDL_Surface * surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
				pixmap.pixels.width(), pixmap.pixels.height(), 32,
				0x00ff0000,
				0x0000ff00,
				0x000000ff,
				0xff000000
				);
		if(SDL_MUSTLOCK(surface)) {
			SDL_LockSurface(surface);
		}
		for(unsigned x = 0; x < pixmap.pixels.width(); ++x) {
			for(unsigned y = 0; y < pixmap.pixels.height(); ++y) {
				Uint8 * pixel = (Uint8*)surface->pixels;
				pixel += (y * surface->pitch) + (x * sizeof(Uint32));
				Uint32 c = pixmap.palette[pixmap.pixels.cell(x, y)];
				*((Uint32*)pixel) = c;
			}
		}
		if(SDL_MUSTLOCK(surface)) {
			SDL_UnlockSurface(surface);
		}
		result = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, pixmap.pixels.width(), pixmap.pixels.height());
		SDL_UpdateTexture(result, 0, surface->pixels, surface->pitch);
		SDL_SetTextureBlendMode(result, SDL_BLENDMODE_BLEND);
	} catch(const Chthon::Pixmap::Exception & e) {
		std::cerr << e.what << std::endl;
	}
	return result;
}

void Font::init(SDL_Renderer * renderer)
{
	font = Sprite::load(renderer, Sprite::font, Chthon::size_of_array(Sprite::font));
}

SDL_Rect Font::getCharRect(char ch) const
{
	SDL_Rect result;
	result.x = ch % 16 * 20;
	result.y = ch / 16 * 20;
	result.w = 20;
	result.h = 20;
	return result;
}

SDL_Texture * Font::getFont() const
{
	return font;
}

