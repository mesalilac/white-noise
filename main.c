#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define HEXCOLOR(code)                                                                                                 \
    ((code) >> (3 * 8)) & 0xFF, ((code) >> (2 * 8)) & 0xFF, ((code) >> (1 * 8)) & 0xFF, ((code) >> (0 * 8)) & 0xFF

#define BACKGROUND_COLOR 0x1F1F28FF
#define WIDTH 800
#define HEIGHT 600

#define FONT_SIZE 18

#define FREQ 4400
#define SAMPLE_DT (1.0f / FREQ)

#define STEP_TIME_MIN 1.0f
#define STEP_TIME_MAX 200.0f

#define STEP_TIME_SLIDER_X 100.0f
#define STEP_TIME_SLIDER_Y 100.0f
#define STEP_TIME_SLIDER_LEN 200.0f
#define STEP_TIME_SLIDER_THICNESS 5.0f
#define STEP_TIME_SLIDER_COLOR 0x363646FF
#define STEP_TIME_SLIDER_TEXT_LEFT_PADDING 15

#define STEP_TIME_SLIDER_GRIP_SIZE 10.0f
#define STEP_TIME_SLIDER_GRIP_COLOR 0x76946AFF
#define STEP_TIME_SLIDER_GRIP_DRAGGING_COLOR 0x5e7655ff

typedef struct
{
    Sint16 current;
    Sint16 next;
    float step_time;
    float a;
} Gen;

void white_noise(Gen *gen, Sint16 *stream, size_t stream_len)
{
    float gen_step = (1.0f / (gen->step_time * SAMPLE_DT));

    for (size_t i = 0; i < stream_len; ++i)
    {
        gen->a += gen_step * SAMPLE_DT;
        stream[i] = (Sint16)floorf((gen->next - gen->current) * cosf(gen->a));

        if (gen->a >= 1.0f)
        {
            Sint16 value = rand() % (1 << 10);
            Sint16 sign = (rand() % 2) * 2 - 1;
            gen->current = gen->next;
            gen->next = value * sign;
            gen->a = 0.0f;
        }
    }
}

void white_noise_callback(void *userdata, Uint8 *stream, int len)
{
    assert(len % 2 == 0);

    white_noise(userdata, (Sint16 *)stream, len / 2);
}

int main()
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "ERROR: Could not initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_Window *window = SDL_CreateWindow("White noise", 0, 0, WIDTH, HEIGHT, 0);

    if (window == NULL)
    {
        fprintf(stderr, "ERROR: Could not create window: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL)
    {
        fprintf(stderr, "ERROR: Could not create renderer: %s\n", SDL_GetError());
        exit(1);
    }

    if (TTF_Init() == -1)
    {
        fprintf(stderr, "ERROR: Could not create TTF: %s", SDL_GetError());
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("FiraCode-Regular.ttf", FONT_SIZE);
    SDL_Color font_color = {255, 255, 255};

    Gen gen = {.step_time = 3.0f};

    SDL_AudioSpec desired = {
        .freq = FREQ, .format = AUDIO_S16LSB, .channels = 1, .callback = white_noise_callback, .userdata = &gen};

    if (SDL_OpenAudio(&desired, NULL) < 0)
    {
        fprintf(stderr, "ERROR: Could not open audio device: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_PauseAudio(0);

    bool quit = false;
    bool dragging_grip = false;
    bool is_playing = true;

    while (!quit)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT: {

                quit = true;
            }
            break;
            case SDL_KEYDOWN: {
                if (event.key.keysym.sym == SDLK_SPACE)
                {
                    is_playing = !is_playing;
                    SDL_PauseAudio(!is_playing);
                }
            }
            break;
            }
        }

        SDL_SetRenderDrawColor(renderer, HEXCOLOR(BACKGROUND_COLOR));
        SDL_RenderClear(renderer);

        {
            SDL_Surface *surfaceMessage =
                TTF_RenderText_Solid(font, is_playing == true ? "PLAYING" : "PAUSED", font_color);

            SDL_Texture *Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

            SDL_SetRenderDrawColor(renderer, HEXCOLOR(BACKGROUND_COLOR));
            SDL_Rect message_rect = {
                .x = WIDTH / 2 - surfaceMessage->w / 2, .y = 5, .w = surfaceMessage->w, .h = surfaceMessage->h};

            SDL_RenderCopy(renderer, Message, NULL, &message_rect);
            SDL_RenderDrawRect(renderer, &message_rect);

            SDL_FreeSurface(surfaceMessage);
            SDL_DestroyTexture(Message);
        }

        // sider body
        {
            SDL_SetRenderDrawColor(renderer, HEXCOLOR(STEP_TIME_SLIDER_COLOR));
            SDL_Rect rect = {
                .x = STEP_TIME_SLIDER_X,
                .y = STEP_TIME_SLIDER_Y - STEP_TIME_SLIDER_THICNESS * 0.5f,
                .w = STEP_TIME_SLIDER_LEN,
                .h = STEP_TIME_SLIDER_THICNESS,

            };
            SDL_RenderFillRect(renderer, &rect);

            int len = snprintf(NULL, 0, "%f", gen.step_time);
            char *step_time_str = malloc(len + 1);
            snprintf(step_time_str, len + 1, "%f", gen.step_time);

            SDL_Surface *surfaceMessage = TTF_RenderText_Solid(font, step_time_str, font_color);

            SDL_Texture *Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

            SDL_SetRenderDrawColor(renderer, HEXCOLOR(BACKGROUND_COLOR));
            SDL_Rect message_rect = {.x =
                                         STEP_TIME_SLIDER_X + STEP_TIME_SLIDER_LEN + STEP_TIME_SLIDER_TEXT_LEFT_PADDING,
                                     .y = STEP_TIME_SLIDER_Y - surfaceMessage->h / 2.0f,
                                     .w = surfaceMessage->w,
                                     .h = surfaceMessage->h};

            SDL_RenderCopy(renderer, Message, NULL, &message_rect);
            SDL_RenderDrawRect(renderer, &message_rect);

            free(step_time_str);
            SDL_FreeSurface(surfaceMessage);
            SDL_DestroyTexture(Message);
        }

        // slider grip

        {
            float grip_value = gen.step_time / (STEP_TIME_MAX - STEP_TIME_MIN) * STEP_TIME_SLIDER_LEN;

            SDL_SetRenderDrawColor(renderer, HEXCOLOR(dragging_grip == true ? STEP_TIME_SLIDER_GRIP_DRAGGING_COLOR
                                                                            : STEP_TIME_SLIDER_GRIP_COLOR));
            SDL_Rect rect = {
                .x = STEP_TIME_SLIDER_X - STEP_TIME_SLIDER_GRIP_SIZE + grip_value,
                .y = STEP_TIME_SLIDER_Y - STEP_TIME_SLIDER_GRIP_SIZE,
                .w = STEP_TIME_SLIDER_GRIP_SIZE * 2.0f,
                .h = STEP_TIME_SLIDER_GRIP_SIZE * 2.0f,
            };

            SDL_RenderFillRect(renderer, &rect);

            int x, y;
            Uint32 buttons = SDL_GetMouseState(&x, &y);

            if (!dragging_grip)
            {
                SDL_Point cursor = {x, y};
                if (SDL_PointInRect(&cursor, &rect) && (buttons & SDL_BUTTON_LMASK) != 0)
                {
                    dragging_grip = true;
                }
            }
            else
            {
                if ((buttons & SDL_BUTTON_LMASK) == 0)
                {
                    dragging_grip = false;
                }
                else
                {
                    float xf = x - STEP_TIME_SLIDER_GRIP_SIZE;
                    float grip_min = STEP_TIME_SLIDER_X - STEP_TIME_SLIDER_GRIP_SIZE;
                    float grip_max = STEP_TIME_SLIDER_X - STEP_TIME_SLIDER_GRIP_SIZE + STEP_TIME_SLIDER_LEN;

                    if (xf < grip_min)
                        xf = grip_min;
                    if (xf > grip_max)
                        xf = grip_max;

                    gen.step_time =
                        (xf - grip_min) / STEP_TIME_SLIDER_LEN * (STEP_TIME_MAX - STEP_TIME_MIN) + STEP_TIME_MIN;
                }
            }
        }

        SDL_RenderPresent(renderer);
    }

    SDL_Quit();

    return 0;
}
