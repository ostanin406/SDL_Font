#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_opengl.h>

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600
#define MINWINDOW 640, 480

static short Width = DEFAULT_WIDTH;
static short Height = DEFAULT_HEIGHT;
TTF_Font* Font;

static void SDL_GL_Enter2DMode(int width, int height)
{
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0.0, (GLdouble)width, (GLdouble)height, 0.0, 0.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

static void SDL_GL_Leave2DMode()
{
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glPopAttrib();
}

#pragma warning(push)
#pragma warning(disable:26812)
static GLuint SDL_GL_LoadTexture(SDL_Surface* surface, SDL_FRect& rect)
{
    int w = 1; while (w < surface->w) w <<= 1;
    int h = 1; while (h < surface->h) h <<= 1;
    rect = { 0.0F, 0.0F, (GLfloat)surface->w / w, (GLfloat)surface->h / h };

    SDL_Surface* image = SDL_CreateRGBSurface(0, w, h, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
    if (image == NULL) return 0;

    Uint8 alpha;
    SDL_BlendMode blend;

    SDL_GetSurfaceAlphaMod(surface, &alpha);
    SDL_SetSurfaceAlphaMod(surface, 0xFF);
    SDL_GetSurfaceBlendMode(surface, &blend);
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);

    SDL_Rect area = { 0, 0, surface->w, surface->h };
    SDL_BlitSurface(surface, &area, image, &area);

    SDL_SetSurfaceAlphaMod(surface, alpha);
    SDL_SetSurfaceBlendMode(surface, blend);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);

    SDL_FreeSurface(image);
    return texture;
}
#pragma warning(pop)

void GL_Init()
{
    glViewport(0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(-2.0, 2.0, -2.0, 2.0, -20.0, 20.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glShadeModel(GL_SMOOTH);
}

SDL_Point SDL_RenderText(TTF_Font* font, const wchar_t* text, SDL_Point point, SDL_Color color)
{
    SDL_Surface* surface = TTF_RenderUNICODE_Blended(font, (const Uint16*)text, color);
    if (surface == NULL) return point;

    SDL_Rect rect = { point.x, point.y, point.x + surface->w, point.y + surface->h };
    SDL_FRect vertex;

    GLuint texture = SDL_GL_LoadTexture(surface, vertex);
    SDL_FreeSurface(surface);

    glBindTexture(GL_TEXTURE_2D, texture);

    glBegin(GL_QUADS);
    glTexCoord2f(vertex.x, vertex.y); glVertex2i(rect.x, rect.y);
    glTexCoord2f(vertex.x, vertex.h); glVertex2i(rect.x, rect.h);
    glTexCoord2f(vertex.w, vertex.h); glVertex2i(rect.w, rect.h);
    glTexCoord2f(vertex.w, vertex.y); glVertex2i(rect.w, rect.y);
    glEnd();

    return SDL_Point{ rect.w, rect.h };
}

void Display(float dt)
{
    glClearColor(0.5F, 0.5F, 0.5F, 0.5F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SDL_GL_Enter2DMode(Width, Height);

    SDL_Point point = { 100, 100 };
    point = SDL_RenderText(Font, L"Text", point, SDL_Color{ 255, 0, 0, 0 });
    point.y = 100;
    SDL_RenderText(Font, L"Текст", point, SDL_Color{ 0, 255, 255, 0 });

    SDL_GL_Leave2DMode();
}

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO)) return 1;
    if (TTF_Init() < 0) return 1;

    Font = TTF_OpenFont("C:/Windows/Fonts/consola.ttf", 24);
    TTF_SetFontStyle(Font, TTF_STYLE_NORMAL);

    SDL_Window* Window = SDL_CreateWindow("Font",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        DEFAULT_WIDTH, DEFAULT_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (Window == NULL) return 1;

    SDL_SetWindowMinimumSize(Window, MINWINDOW);
    SDL_GLContext GLContext = SDL_GL_CreateContext(Window);
    SDL_GL_SetSwapInterval(0);

    GL_Init();

    Uint32 time, pretime = 0;
    float deltatime, dtrest;

    SDL_bool running = SDL_TRUE;
    SDL_Event event;

    while (running == SDL_TRUE)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT: running = SDL_FALSE; break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    Width = event.window.data1;
                    Height = event.window.data2;
                }
                break;
            default: break;
            }
        }

        time = SDL_GetTicks();
        deltatime = 0.001F * (float)(time - pretime);
        pretime = time;

        Display(deltatime);
        SDL_GL_SwapWindow(Window);

        dtrest = roundf(1.0F / 30.0F - deltatime);
        if (dtrest > 0.0F) SDL_Delay((Uint32)(dtrest * 1000));
    }

    SDL_GL_DeleteContext(GLContext);
    SDL_DestroyWindow(Window);

    TTF_CloseFont(Font);
    TTF_Quit();
    SDL_Quit();
    return 0;
}