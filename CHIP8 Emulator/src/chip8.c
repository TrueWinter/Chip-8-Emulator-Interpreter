#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "SDL2/SDL.h"

#define WIDTH 1024
#define HEIGHT 512

typedef struct {
	uint8_t memory[4096];
	uint8_t V[16]; //Registers V0-V15, 16th is used for carry flag 

	uint16_t I; //Index register
	uint16_t pc; //Program counter
	uint16_t stack[16]; //Stack for jmps
	uint16_t sp; //Stack pointer

	uint8_t key[16]; //Hex based keypad

    uint16_t  opcode; //Current opcode

    uint8_t gfx[2048];
    uint8_t delay_timer;
    uint8_t sound_timer;

    int drawFlag;
} chip8;

unsigned char chip8_fontset[80] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

// Keypad keymap
uint8_t keymap[16] = {
    SDLK_x,
    SDLK_1,
    SDLK_2,
    SDLK_3,
    SDLK_q,
    SDLK_w,
    SDLK_e,
    SDLK_a,
    SDLK_s,
    SDLK_d,
    SDLK_z,
    SDLK_c,
    SDLK_4,
    SDLK_r,
    SDLK_f,
    SDLK_v,
};



//Prototypes
void initCHIP8(chip8* CPU);
void dumpMemory(chip8* CPU);
void dumpValues(chip8* CPU);
void loadProgramIntoMemory(chip8* CPU);
void emulateCycle(chip8* CPU);

int main(int argc, char* argv[]){

    SDL_Window *window = NULL;
    SDL_Init(SDL_INIT_EVERYTHING);

    window = SDL_CreateWindow(
            "CHIP-8 Emulator",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            WIDTH, HEIGHT, SDL_WINDOW_SHOWN
    );

    // Create renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT);

     // Create texture that stores frame buffer
    SDL_Texture* sdlTexture = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            64, 32);

    // Temporary pixel buffer
    uint32_t pixels[2048];

	chip8 CPU;

	initCHIP8(&CPU);
	loadProgramIntoMemory(&CPU);


    //Emulation loop
    for(;;){
  
        emulateCycle(&CPU);

         // Process SDL events
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) exit(0);

            // Process keydown events
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                    exit(0);


                for (int i = 0; i < 16; ++i) {
                    if (e.key.keysym.sym == keymap[i]) {
                        CPU.key[i] = 1;
                    }
                }
            }

             // Process keyup events
            if (e.type == SDL_KEYUP) {
                for (int i = 0; i < 16; ++i) {
                    if (e.key.keysym.sym == keymap[i]) {
                        CPU.key[i] = 0;
                    }
                }
            }
        }


       // if(CPU.key[0] == 1){
       //     CPU.key[0] = 0;
     //       emulateCycle(&CPU);
     //   }



        if(CPU.drawFlag == 1){
            CPU.drawFlag = 0;

             // Store pixels in temporary buffer
            for (int i = 0; i < 2048; ++i) {
                uint8_t pixel = CPU.gfx[i];
                pixels[i] = (0x00FFFFFF * pixel) | 0xFF000000;
            }
            // Update SDL texture
            SDL_UpdateTexture(sdlTexture, NULL, pixels, 64 * sizeof(Uint32));
            // Clear screen and render
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, sdlTexture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }

        usleep(10 * 150);
    }

	return 0;
}

void emulateCycle(chip8* CPU){

    //Support for 15/16 opcodes

   dumpValues(CPU);
   printf("979: %d\n", CPU->memory[979]);

   CPU->opcode = CPU->memory[CPU->pc] << 8 | CPU->memory[CPU->pc + 1];   // Op code is two bytes

   printf("Running opcode: %X\n", CPU->opcode);
   

    switch(CPU->opcode & 0xF000){

         case 0x0000:
            switch(CPU->opcode & 0x000F)
            {
              case 0x0000: // 0x00E0: Clears the screen        
                for(int i = 0; i < 2048; i++){
                    CPU->gfx[i] = 0;
                }

               CPU->drawFlag = 1;
               CPU->pc += 2;
              break;
         
              case 0x000E: // 0x00EE: Returns from subroutine          
               CPU->sp = CPU->sp - 1;
               CPU->pc = CPU->stack[CPU->sp];
               CPU->pc += 2;
              break;

               default:
                printf ("Unknown opcode [0x0000]: 0x%X\n",  CPU->opcode);
                exit(3);          
            }
        break; 

        //Jumps
        case 0x1000:
            CPU->pc = CPU->opcode & 0x0FFF;
        break;

         // 2NNN - Calls subroutine at NNN
        case 0x2000:
                CPU->stack[CPU->sp] = CPU->pc;
                CPU->sp = CPU->sp +1;
                CPU->pc = CPU->opcode & 0x0FFF;
            break;

       // 3XNN - Skips the next instruction if VX equals NN.
        case 0x3000:
            if (CPU->V[(CPU->opcode & 0x0F00) >> 8] == (CPU->opcode & 0x00FF))
                CPU->pc += 4;
            else
                CPU->pc += 2;
            break;

        // 4XNN - Skips the next instruction if VX does not equal NN.
        case 0x4000:
            if (CPU->V[(CPU->opcode & 0x0F00) >> 8] != (CPU->opcode & 0x00FF))
                CPU->pc += 4;
            else
                CPU->pc += 2;
            break;

        //Sets VX to NN
        case 0x6000:
            CPU->V[(CPU->opcode & 0x0F00) >> 8] = CPU->opcode & 0x00FF;
            CPU->pc += 2;
        break;

        //Adds NN to NX
        case 0x7000:
         CPU->V[(CPU->opcode & 0x0F00) >> 8] += CPU->opcode & 0x00FF;
         CPU->pc += 2;
        break;

        case 0x8000:
         switch(CPU->opcode & 0xF00F){
            //Set VX to VY
            case 0x8000:{
                 CPU->V[(CPU->opcode & 0x0F00) >> 8] = (CPU->V[(CPU->opcode & 0x00F0) >> 4]); //FIGURE OUT WHY 4
                 CPU->pc += 2;
             }
            break;

            //Sets VX to VX & VY
            case 0x8002:
                CPU->V[((CPU->opcode & 0x0F00) >> 8)] =  CPU->V[((CPU->opcode & 0x0F00) >> 8)] & CPU->V[((CPU->opcode & 0x00F0) >> 8)];
                CPU->pc += 2;
            break;

            case 0x8003:
                CPU->V[(CPU->opcode & 0x0F00) >> 8] ^= CPU->V[(CPU->opcode & 0x00F0) >> 4];
                CPU->pc += 2;
            break;

            //Adds VY to VX
            case 0x8004:
                CPU->V[(CPU->opcode & 0x0F00) >> 8] += CPU->V[(CPU->opcode & 0x00F0) >> 4];
                    if(CPU->V[(CPU->opcode & 0x00F0) >> 4] > (0xFF - CPU->V[(CPU->opcode & 0x0F00) >> 8]))
                        CPU->V[0xF] = 1; //carry
                    else
                        CPU->V[0xF] = 0;
                    CPU->pc += 2;
            break;

            case 0x8005:
                    if(CPU->V[(CPU->opcode & 0x00F0) >> 4] > CPU->V[(CPU->opcode & 0x0F00) >> 8])
                        CPU->V[0xF] = 0; // there is a borrow
                    else
                        CPU->V[0xF] = 1;
                    CPU->V[(CPU->opcode & 0x0F00) >> 8] -= CPU->V[(CPU->opcode & 0x00F0) >> 4];
                    CPU->pc += 2;
            break;

            case 0x8006:
                CPU->V[15] = CPU->V[(CPU->opcode & 0x0F00) >> 8] & 0x1;
                CPU->V[(CPU->opcode & 0x0F00) >> 8] >>= 1;
                CPU->pc += 2;
             break;

            break;

            default:
                 printf ("Unknown opcode [0x8000]: 0x%X\n",  CPU->opcode);
                  exit(3);
         }
        break;

        //Sets I to ADDRESS of 0NNN
        case 0xA000:
            CPU->I = CPU->opcode & 0x0FFF; 
            CPU->pc += 2;
        break;

        //Draws sprite on screen
        case 0xD000:{
              unsigned short x = CPU->V[(CPU->opcode & 0x0F00) >> 8];
              unsigned short y = CPU->V[(CPU->opcode & 0x00F0) >> 4];
              unsigned short height = CPU->opcode & 0x000F;
              unsigned short pixel;
             
              CPU->V[15] = 0;
              for (int yline = 0; yline < height; yline++)
              {
                pixel = CPU->memory[CPU->I + yline];
                for(int xline = 0; xline < 8; xline++)
                {
                  if((pixel & (0x80 >> xline)) != 0)
                  {
                    if(CPU->gfx[(x + xline + ((y + yline) * 64))] == 1)
                      CPU->V[15] = 1;                                 
                    CPU->gfx[x + xline + ((y + yline) * 64)] ^= 1;
                  }
                }
              }

            CPU->drawFlag = 1;
            CPU->pc += 2;
            }
        break;


        case 0xE000:
            switch(CPU->opcode & 0xF00F){
                case 0xE001:
                 if(CPU->key[CPU->V[(CPU->opcode & 0x0F00) >> 8]] == 0)
                    CPU->pc += 4;
                 else
                     CPU->pc += 2;
                break;

                //Skips next instruction is key in VX is pressed.
                case 0xE00E:
                 if(CPU->key[CPU->V[(CPU->opcode & 0x0F00) >> 8]] == 1)
                    CPU->pc += 4;
                 else
                     CPU->pc += 2;
                break;

                default:
                 printf ("Unknown opcode [0xE000]: 0x%X\n",  CPU->opcode);
                  exit(3);
            }
        break;


        //Add VX to I
        case 0xF000:
            switch(CPU->opcode & 0xF0FF){

                 // FX1E - Adds VX to I
                case 0xF01E:
                    // VF is set to 1 when range overflow (I+VX>0xFFF), and 0
                    // when there isn't.
                    if(CPU->I + CPU->V[(CPU->opcode & 0x0F00) >> 8] > 0xFFF)
                        CPU->V[0xF] = 1;
                    else
                        CPU->V[0xF] = 0;
                    CPU->I += CPU->V[(CPU->opcode & 0x0F00) >> 8];
                    CPU->pc += 2;
                    break;

                case 0xF007:
                  CPU->V[(CPU->opcode & 0x0F00)] = CPU->delay_timer;
                  CPU->pc += 2;
                break;


                //Sets delay timer to VX
                case 0xF015:
                 CPU->delay_timer = CPU->V[(CPU->opcode & 0x0F00)];
                 CPU->pc += 2;
                break;

                case 0xF018:
                    CPU->sound_timer = CPU->V[(CPU->opcode & 0x0F00) >> 8];
                    CPU->pc += 2;
                break;

                 case 0xF065: //FX65: Fills V0 to VX with values from memory starting at address I
                        for(int i = 0; i <= ((CPU->opcode & 0x0F00) >> 8); i++)
                            CPU->V[i] = CPU->memory[CPU->I + i];
                        CPU->pc += 2;
                    break;



                default:
                 printf ("Unknown opcode [0xF000]: 0x%X\n",  CPU->opcode);
                  exit(3);
            }
        break;

        default:
                printf ("Unknown opcode: 0x%X\n",  CPU->opcode);
                exit(3);
    }


 // Update timers
    if (CPU->delay_timer > 0)
        CPU->delay_timer = CPU->delay_timer -1;
   
}

/*
* Clears memory, registers and screen
*/
void initCHIP8(chip8* CPU){

    CPU->pc = 0x200;
    CPU->opcode = 0;
    CPU->I = 0;
    CPU->sp = 0;

	int i;

	//Clear memory
	for(i = 0; i < 4096; i++){
		CPU->memory[i] = 0x00;
	}

	//Clear registers, stack and keypad
	for(i = 0; i < 15; i++){
		CPU->V[i] = 0;
        CPU->key[i] = 0;
        CPU->stack[i] = 0;
	}

	//Clear screen
	for(i = 0; i < 64*32; i++){
		CPU->gfx[i] = 0x00;
	}

	 // Load font set into memory
    for (int i = 0; i < 80; ++i) {
        CPU->memory[i] = chip8_fontset[i];
    }

    CPU->delay_timer = 0;
    CPU->sound_timer = 0;
}

/*
* Dumps memory as int representations of char bytes
*/
void dumpMemory(chip8* CPU){

	int i,j;

    printf("\n");
	//Prints memory byte by byte
	for(i = 0; i < 4096; i++){
		printf("Byte %d:", i);
		for(j = 0; j < 8; j++){
			printf("%d",!!((CPU->memory[i] << j) & 0x80));
		}
		printf("\n");
	}
}

void dumpValues(chip8* CPU){
    printf("PC: %d\n", CPU->pc);
    printf("I: %d\n", CPU->I);
    printf("V0: %d\n", CPU->V[0]);
    printf("V1: %d\n", CPU->V[1]);
    printf("V2: %d\n", CPU->V[2]);
}

void loadProgramIntoMemory(chip8* CPU){
	FILE *fp;

 	printf("\nReading ROM...\n");

	//Read in binary mode
	//fp = fopen("INVADERS", "rb");
    //fp = fopen("BC_test.ch8", "rb");
    fp = fopen("Space Invaders.ch8", "rb");

	  if (fp == NULL) {
         printf("Failed to open ROM.\n");
    }

	// Get file size
    fseek(fp, 0, SEEK_END);
    long rom_size = ftell(fp);
    rewind(fp);
    printf("SIZE: %ld bytes\n", rom_size);

    // Allocate memory to store rom
    char* rom_buffer = (char*) malloc(sizeof(char) * rom_size);
    if (rom_buffer == NULL) {
        printf("Failed to allocate memory for ROM.\n");
    }

    // Copy ROM into buffer
    size_t result = fread(rom_buffer, sizeof(char), (size_t)rom_size, fp);
    if (result != rom_size) {
        printf("Failed to read ROM.\n");
    }

    int i;

    // Copy buffer to memory
      if ((4096-512) > rom_size){
        for (i = 0; i < rom_size; ++i) {
            CPU->memory[i + 512] = (uint8_t)rom_buffer[i];   // Load into memory starting          // at 0x200 (=512)
        }
    }else{
        printf("ROM too large to fit in memory..Size of: %ld/3584", rom_size);
    }

	fclose(fp);
    free(rom_buffer);
}


