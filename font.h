#pragma once
class SDL_Texture;
class SDL_Renderer;
class SDL_Rect;

class Font {
public:
	Font() : font(0) {}
	void init(SDL_Renderer * renderer);
	SDL_Rect getCharRect(char ch) const;
	SDL_Texture * getFont() const;
private:
	SDL_Texture * font;
};
