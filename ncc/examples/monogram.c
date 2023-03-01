#include <uvm/syscalls.h>
#include <uvm/utils.h>


// Monogram 12x7 pixel font

// > # CREDITS
// > 
// > Monogram is a free and Creative Commons Zero pixel font,
// > made by Vinícius Menézio (@vmenezio).
// > 
// > https://datagoblin.itch.io/monogram
// > 
// > 
// > # SPECIAL THANKS
// > 
// > thanks to Ateş Göral (@atesgoral) for creating the bitmap font converter:
// > https://codepen.io/atesgoral/details/RwGOvPZ
// > 
// > thanks to Éric Araujo (@merwok_) for the inital port of monogram to PICO-8:
// > https://itch.io/post/2625522

u64 font_monogram_number_of_characters = 390;
u64 font_monogram_height = 12;
u64 font_monogram_width = 7;

u8 font_monogram_data[390][12];

// Map from ord(ch) to index of ch's data in font_monogram_data.
// This is for when you know the character that you want to draw but not
// its index in the font_monogram_data array.
u16 font_monogram_chars[8595];


void
init_monogram_font_data()
{
    memset(font_monogram_data, 0, sizeof(font_monogram_data));

    // Empty slots initialized to -1.
    memset(font_monogram_chars, 0xFF, sizeof(font_monogram_chars));

    font_monogram_data[0][0] = 0;
    // '0'
    font_monogram_data[0][ 3] = 0b0001110;
    font_monogram_data[0][ 4] = 0b0010001;
    font_monogram_data[0][ 5] = 0b0011001;
    font_monogram_data[0][ 6] = 0b0010101;
    font_monogram_data[0][ 7] = 0b0010011;
    font_monogram_data[0][ 8] = 0b0010001;
    font_monogram_data[0][ 9] = 0b0001110;
    font_monogram_chars[0x30] = 0;
    // '1'
    font_monogram_data[1][ 3] = 0b0000100;
    font_monogram_data[1][ 4] = 0b0000110;
    font_monogram_data[1][ 5] = 0b0000100;
    font_monogram_data[1][ 6] = 0b0000100;
    font_monogram_data[1][ 7] = 0b0000100;
    font_monogram_data[1][ 8] = 0b0000100;
    font_monogram_data[1][ 9] = 0b0011111;
    font_monogram_chars[0x31] = 1;
    // '2'
    font_monogram_data[2][ 3] = 0b0001110;
    font_monogram_data[2][ 4] = 0b0010001;
    font_monogram_data[2][ 5] = 0b0010000;
    font_monogram_data[2][ 6] = 0b0001000;
    font_monogram_data[2][ 7] = 0b0000100;
    font_monogram_data[2][ 8] = 0b0000010;
    font_monogram_data[2][ 9] = 0b0011111;
    font_monogram_chars[0x32] = 2;
    // '3'
    font_monogram_data[3][ 3] = 0b0001110;
    font_monogram_data[3][ 4] = 0b0010001;
    font_monogram_data[3][ 5] = 0b0010000;
    font_monogram_data[3][ 6] = 0b0001100;
    font_monogram_data[3][ 7] = 0b0010000;
    font_monogram_data[3][ 8] = 0b0010001;
    font_monogram_data[3][ 9] = 0b0001110;
    font_monogram_chars[0x33] = 3;
    // '4'
    font_monogram_data[4][ 3] = 0b0010010;
    font_monogram_data[4][ 4] = 0b0010010;
    font_monogram_data[4][ 5] = 0b0010001;
    font_monogram_data[4][ 6] = 0b0011111;
    font_monogram_data[4][ 7] = 0b0010000;
    font_monogram_data[4][ 8] = 0b0010000;
    font_monogram_data[4][ 9] = 0b0010000;
    font_monogram_chars[0x34] = 4;
    // '5'
    font_monogram_data[5][ 3] = 0b0011111;
    font_monogram_data[5][ 4] = 0b0000001;
    font_monogram_data[5][ 5] = 0b0001111;
    font_monogram_data[5][ 6] = 0b0010000;
    font_monogram_data[5][ 7] = 0b0010000;
    font_monogram_data[5][ 8] = 0b0010001;
    font_monogram_data[5][ 9] = 0b0001110;
    font_monogram_chars[0x35] = 5;
    // '6'
    font_monogram_data[6][ 3] = 0b0001110;
    font_monogram_data[6][ 4] = 0b0000001;
    font_monogram_data[6][ 5] = 0b0000001;
    font_monogram_data[6][ 6] = 0b0001111;
    font_monogram_data[6][ 7] = 0b0010001;
    font_monogram_data[6][ 8] = 0b0010001;
    font_monogram_data[6][ 9] = 0b0001110;
    font_monogram_chars[0x36] = 6;
    // '7'
    font_monogram_data[7][ 3] = 0b0011111;
    font_monogram_data[7][ 4] = 0b0010000;
    font_monogram_data[7][ 5] = 0b0010000;
    font_monogram_data[7][ 6] = 0b0001000;
    font_monogram_data[7][ 7] = 0b0000100;
    font_monogram_data[7][ 8] = 0b0000100;
    font_monogram_data[7][ 9] = 0b0000100;
    font_monogram_chars[0x37] = 7;
    // '8'
    font_monogram_data[8][ 3] = 0b0001110;
    font_monogram_data[8][ 4] = 0b0010001;
    font_monogram_data[8][ 5] = 0b0010001;
    font_monogram_data[8][ 6] = 0b0001110;
    font_monogram_data[8][ 7] = 0b0010001;
    font_monogram_data[8][ 8] = 0b0010001;
    font_monogram_data[8][ 9] = 0b0001110;
    font_monogram_chars[0x38] = 8;
    // '9'
    font_monogram_data[9][ 3] = 0b0001110;
    font_monogram_data[9][ 4] = 0b0010001;
    font_monogram_data[9][ 5] = 0b0010001;
    font_monogram_data[9][ 6] = 0b0011110;
    font_monogram_data[9][ 7] = 0b0010000;
    font_monogram_data[9][ 8] = 0b0010001;
    font_monogram_data[9][ 9] = 0b0001110;
    font_monogram_chars[0x39] = 9;
    // '!'
    font_monogram_data[10][ 3] = 0b0000100;
    font_monogram_data[10][ 4] = 0b0000100;
    font_monogram_data[10][ 5] = 0b0000100;
    font_monogram_data[10][ 6] = 0b0000100;
    font_monogram_data[10][ 7] = 0b0000100;
    font_monogram_data[10][ 9] = 0b0000100;
    font_monogram_chars[0x21] = 10;
    // '"'
    font_monogram_data[11][ 3] = 0b0001010;
    font_monogram_data[11][ 4] = 0b0001010;
    font_monogram_data[11][ 5] = 0b0001010;
    font_monogram_chars[0x22] = 11;
    // '#'
    font_monogram_data[12][ 4] = 0b0001010;
    font_monogram_data[12][ 5] = 0b0011111;
    font_monogram_data[12][ 6] = 0b0001010;
    font_monogram_data[12][ 7] = 0b0001010;
    font_monogram_data[12][ 8] = 0b0011111;
    font_monogram_data[12][ 9] = 0b0001010;
    font_monogram_chars[0x23] = 12;
    // '$'
    font_monogram_data[13][ 3] = 0b0000100;
    font_monogram_data[13][ 4] = 0b0011110;
    font_monogram_data[13][ 5] = 0b0000101;
    font_monogram_data[13][ 6] = 0b0001110;
    font_monogram_data[13][ 7] = 0b0010100;
    font_monogram_data[13][ 8] = 0b0001111;
    font_monogram_data[13][ 9] = 0b0000100;
    font_monogram_chars[0x24] = 13;
    // '%'
    font_monogram_data[14][ 3] = 0b0010001;
    font_monogram_data[14][ 4] = 0b0010001;
    font_monogram_data[14][ 5] = 0b0001000;
    font_monogram_data[14][ 6] = 0b0000100;
    font_monogram_data[14][ 7] = 0b0000010;
    font_monogram_data[14][ 8] = 0b0010001;
    font_monogram_data[14][ 9] = 0b0010001;
    font_monogram_chars[0x25] = 14;
    // '&'
    font_monogram_data[15][ 3] = 0b0000110;
    font_monogram_data[15][ 4] = 0b0001001;
    font_monogram_data[15][ 5] = 0b0001001;
    font_monogram_data[15][ 6] = 0b0011110;
    font_monogram_data[15][ 7] = 0b0001001;
    font_monogram_data[15][ 8] = 0b0001001;
    font_monogram_data[15][ 9] = 0b0010110;
    font_monogram_chars[0x26] = 15;
    // "'"
    font_monogram_data[16][ 3] = 0b0000100;
    font_monogram_data[16][ 4] = 0b0000100;
    font_monogram_data[16][ 5] = 0b0000100;
    font_monogram_chars[0x27] = 16;
    // '('
    font_monogram_data[17][ 3] = 0b0001000;
    font_monogram_data[17][ 4] = 0b0000100;
    font_monogram_data[17][ 5] = 0b0000100;
    font_monogram_data[17][ 6] = 0b0000100;
    font_monogram_data[17][ 7] = 0b0000100;
    font_monogram_data[17][ 8] = 0b0000100;
    font_monogram_data[17][ 9] = 0b0001000;
    font_monogram_chars[0x28] = 17;
    // ')'
    font_monogram_data[18][ 3] = 0b0000010;
    font_monogram_data[18][ 4] = 0b0000100;
    font_monogram_data[18][ 5] = 0b0000100;
    font_monogram_data[18][ 6] = 0b0000100;
    font_monogram_data[18][ 7] = 0b0000100;
    font_monogram_data[18][ 8] = 0b0000100;
    font_monogram_data[18][ 9] = 0b0000010;
    font_monogram_chars[0x29] = 18;
    // '*'
    font_monogram_data[19][ 4] = 0b0000100;
    font_monogram_data[19][ 5] = 0b0010101;
    font_monogram_data[19][ 6] = 0b0001110;
    font_monogram_data[19][ 7] = 0b0010101;
    font_monogram_data[19][ 8] = 0b0000100;
    font_monogram_chars[0x2a] = 19;
    // '+'
    font_monogram_data[20][ 4] = 0b0000100;
    font_monogram_data[20][ 5] = 0b0000100;
    font_monogram_data[20][ 6] = 0b0011111;
    font_monogram_data[20][ 7] = 0b0000100;
    font_monogram_data[20][ 8] = 0b0000100;
    font_monogram_chars[0x2b] = 20;
    // ','
    font_monogram_data[21][ 8] = 0b0000100;
    font_monogram_data[21][ 9] = 0b0000100;
    font_monogram_data[21][10] = 0b0000010;
    font_monogram_chars[0x2c] = 21;
    // '-'
    font_monogram_data[22][ 6] = 0b0011111;
    font_monogram_chars[0x2d] = 22;
    // '.'
    font_monogram_data[23][ 8] = 0b0000100;
    font_monogram_data[23][ 9] = 0b0000100;
    font_monogram_chars[0x2e] = 23;
    // '/'
    font_monogram_data[24][ 3] = 0b0010000;
    font_monogram_data[24][ 4] = 0b0010000;
    font_monogram_data[24][ 5] = 0b0001000;
    font_monogram_data[24][ 6] = 0b0000100;
    font_monogram_data[24][ 7] = 0b0000010;
    font_monogram_data[24][ 8] = 0b0000001;
    font_monogram_data[24][ 9] = 0b0000001;
    font_monogram_chars[0x2f] = 24;
    // ':'
    font_monogram_data[25][ 4] = 0b0000100;
    font_monogram_data[25][ 5] = 0b0000100;
    font_monogram_data[25][ 8] = 0b0000100;
    font_monogram_data[25][ 9] = 0b0000100;
    font_monogram_chars[0x3a] = 25;
    // ';'
    font_monogram_data[26][ 4] = 0b0000100;
    font_monogram_data[26][ 5] = 0b0000100;
    font_monogram_data[26][ 8] = 0b0000100;
    font_monogram_data[26][ 9] = 0b0000100;
    font_monogram_data[26][10] = 0b0000010;
    font_monogram_chars[0x3b] = 26;
    // '<'
    font_monogram_data[27][ 4] = 0b0011000;
    font_monogram_data[27][ 5] = 0b0000110;
    font_monogram_data[27][ 6] = 0b0000001;
    font_monogram_data[27][ 7] = 0b0000110;
    font_monogram_data[27][ 8] = 0b0011000;
    font_monogram_chars[0x3c] = 27;
    // '='
    font_monogram_data[28][ 5] = 0b0011111;
    font_monogram_data[28][ 7] = 0b0011111;
    font_monogram_chars[0x3d] = 28;
    // '>'
    font_monogram_data[29][ 4] = 0b0000011;
    font_monogram_data[29][ 5] = 0b0001100;
    font_monogram_data[29][ 6] = 0b0010000;
    font_monogram_data[29][ 7] = 0b0001100;
    font_monogram_data[29][ 8] = 0b0000011;
    font_monogram_chars[0x3e] = 29;
    // '?'
    font_monogram_data[30][ 3] = 0b0001110;
    font_monogram_data[30][ 4] = 0b0010001;
    font_monogram_data[30][ 5] = 0b0010000;
    font_monogram_data[30][ 6] = 0b0001000;
    font_monogram_data[30][ 7] = 0b0000100;
    font_monogram_data[30][ 9] = 0b0000100;
    font_monogram_chars[0x3f] = 30;
    // '@'
    font_monogram_data[31][ 3] = 0b0001110;
    font_monogram_data[31][ 4] = 0b0011001;
    font_monogram_data[31][ 5] = 0b0010101;
    font_monogram_data[31][ 6] = 0b0010101;
    font_monogram_data[31][ 7] = 0b0011001;
    font_monogram_data[31][ 8] = 0b0000001;
    font_monogram_data[31][ 9] = 0b0001110;
    font_monogram_chars[0x40] = 31;
    // 'A'
    font_monogram_data[32][ 3] = 0b0001110;
    font_monogram_data[32][ 4] = 0b0010001;
    font_monogram_data[32][ 5] = 0b0010001;
    font_monogram_data[32][ 6] = 0b0010001;
    font_monogram_data[32][ 7] = 0b0011111;
    font_monogram_data[32][ 8] = 0b0010001;
    font_monogram_data[32][ 9] = 0b0010001;
    font_monogram_chars[0x41] = 32;
    // 'B'
    font_monogram_data[33][ 3] = 0b0001111;
    font_monogram_data[33][ 4] = 0b0010001;
    font_monogram_data[33][ 5] = 0b0010001;
    font_monogram_data[33][ 6] = 0b0001111;
    font_monogram_data[33][ 7] = 0b0010001;
    font_monogram_data[33][ 8] = 0b0010001;
    font_monogram_data[33][ 9] = 0b0001111;
    font_monogram_chars[0x42] = 33;
    // 'C'
    font_monogram_data[34][ 3] = 0b0001110;
    font_monogram_data[34][ 4] = 0b0010001;
    font_monogram_data[34][ 5] = 0b0000001;
    font_monogram_data[34][ 6] = 0b0000001;
    font_monogram_data[34][ 7] = 0b0000001;
    font_monogram_data[34][ 8] = 0b0010001;
    font_monogram_data[34][ 9] = 0b0001110;
    font_monogram_chars[0x43] = 34;
    // 'D'
    font_monogram_data[35][ 3] = 0b0001111;
    font_monogram_data[35][ 4] = 0b0010001;
    font_monogram_data[35][ 5] = 0b0010001;
    font_monogram_data[35][ 6] = 0b0010001;
    font_monogram_data[35][ 7] = 0b0010001;
    font_monogram_data[35][ 8] = 0b0010001;
    font_monogram_data[35][ 9] = 0b0001111;
    font_monogram_chars[0x44] = 35;
    // 'E'
    font_monogram_data[36][ 3] = 0b0011111;
    font_monogram_data[36][ 4] = 0b0000001;
    font_monogram_data[36][ 5] = 0b0000001;
    font_monogram_data[36][ 6] = 0b0001111;
    font_monogram_data[36][ 7] = 0b0000001;
    font_monogram_data[36][ 8] = 0b0000001;
    font_monogram_data[36][ 9] = 0b0011111;
    font_monogram_chars[0x45] = 36;
    // 'F'
    font_monogram_data[37][ 3] = 0b0011111;
    font_monogram_data[37][ 4] = 0b0000001;
    font_monogram_data[37][ 5] = 0b0000001;
    font_monogram_data[37][ 6] = 0b0001111;
    font_monogram_data[37][ 7] = 0b0000001;
    font_monogram_data[37][ 8] = 0b0000001;
    font_monogram_data[37][ 9] = 0b0000001;
    font_monogram_chars[0x46] = 37;
    // 'G'
    font_monogram_data[38][ 3] = 0b0001110;
    font_monogram_data[38][ 4] = 0b0010001;
    font_monogram_data[38][ 5] = 0b0000001;
    font_monogram_data[38][ 6] = 0b0011101;
    font_monogram_data[38][ 7] = 0b0010001;
    font_monogram_data[38][ 8] = 0b0010001;
    font_monogram_data[38][ 9] = 0b0001110;
    font_monogram_chars[0x47] = 38;
    // 'H'
    font_monogram_data[39][ 3] = 0b0010001;
    font_monogram_data[39][ 4] = 0b0010001;
    font_monogram_data[39][ 5] = 0b0010001;
    font_monogram_data[39][ 6] = 0b0011111;
    font_monogram_data[39][ 7] = 0b0010001;
    font_monogram_data[39][ 8] = 0b0010001;
    font_monogram_data[39][ 9] = 0b0010001;
    font_monogram_chars[0x48] = 39;
    // 'I'
    font_monogram_data[40][ 3] = 0b0011111;
    font_monogram_data[40][ 4] = 0b0000100;
    font_monogram_data[40][ 5] = 0b0000100;
    font_monogram_data[40][ 6] = 0b0000100;
    font_monogram_data[40][ 7] = 0b0000100;
    font_monogram_data[40][ 8] = 0b0000100;
    font_monogram_data[40][ 9] = 0b0011111;
    font_monogram_chars[0x49] = 40;
    // 'J'
    font_monogram_data[41][ 3] = 0b0010000;
    font_monogram_data[41][ 4] = 0b0010000;
    font_monogram_data[41][ 5] = 0b0010000;
    font_monogram_data[41][ 6] = 0b0010000;
    font_monogram_data[41][ 7] = 0b0010001;
    font_monogram_data[41][ 8] = 0b0010001;
    font_monogram_data[41][ 9] = 0b0001110;
    font_monogram_chars[0x4a] = 41;
    // 'K'
    font_monogram_data[42][ 3] = 0b0010001;
    font_monogram_data[42][ 4] = 0b0001001;
    font_monogram_data[42][ 5] = 0b0000101;
    font_monogram_data[42][ 6] = 0b0000011;
    font_monogram_data[42][ 7] = 0b0000101;
    font_monogram_data[42][ 8] = 0b0001001;
    font_monogram_data[42][ 9] = 0b0010001;
    font_monogram_chars[0x4b] = 42;
    // 'L'
    font_monogram_data[43][ 3] = 0b0000001;
    font_monogram_data[43][ 4] = 0b0000001;
    font_monogram_data[43][ 5] = 0b0000001;
    font_monogram_data[43][ 6] = 0b0000001;
    font_monogram_data[43][ 7] = 0b0000001;
    font_monogram_data[43][ 8] = 0b0000001;
    font_monogram_data[43][ 9] = 0b0011111;
    font_monogram_chars[0x4c] = 43;
    // 'M'
    font_monogram_data[44][ 3] = 0b0010001;
    font_monogram_data[44][ 4] = 0b0011011;
    font_monogram_data[44][ 5] = 0b0010101;
    font_monogram_data[44][ 6] = 0b0010001;
    font_monogram_data[44][ 7] = 0b0010001;
    font_monogram_data[44][ 8] = 0b0010001;
    font_monogram_data[44][ 9] = 0b0010001;
    font_monogram_chars[0x4d] = 44;
    // 'N'
    font_monogram_data[45][ 3] = 0b0010001;
    font_monogram_data[45][ 4] = 0b0010001;
    font_monogram_data[45][ 5] = 0b0010011;
    font_monogram_data[45][ 6] = 0b0010101;
    font_monogram_data[45][ 7] = 0b0011001;
    font_monogram_data[45][ 8] = 0b0010001;
    font_monogram_data[45][ 9] = 0b0010001;
    font_monogram_chars[0x4e] = 45;
    // 'O'
    font_monogram_data[46][ 3] = 0b0001110;
    font_monogram_data[46][ 4] = 0b0010001;
    font_monogram_data[46][ 5] = 0b0010001;
    font_monogram_data[46][ 6] = 0b0010001;
    font_monogram_data[46][ 7] = 0b0010001;
    font_monogram_data[46][ 8] = 0b0010001;
    font_monogram_data[46][ 9] = 0b0001110;
    font_monogram_chars[0x4f] = 46;
    // 'P'
    font_monogram_data[47][ 3] = 0b0001111;
    font_monogram_data[47][ 4] = 0b0010001;
    font_monogram_data[47][ 5] = 0b0010001;
    font_monogram_data[47][ 6] = 0b0001111;
    font_monogram_data[47][ 7] = 0b0000001;
    font_monogram_data[47][ 8] = 0b0000001;
    font_monogram_data[47][ 9] = 0b0000001;
    font_monogram_chars[0x50] = 47;
    // 'Q'
    font_monogram_data[48][ 3] = 0b0001110;
    font_monogram_data[48][ 4] = 0b0010001;
    font_monogram_data[48][ 5] = 0b0010001;
    font_monogram_data[48][ 6] = 0b0010001;
    font_monogram_data[48][ 7] = 0b0010001;
    font_monogram_data[48][ 8] = 0b0010001;
    font_monogram_data[48][ 9] = 0b0001110;
    font_monogram_data[48][10] = 0b0011000;
    font_monogram_chars[0x51] = 48;
    // 'R'
    font_monogram_data[49][ 3] = 0b0001111;
    font_monogram_data[49][ 4] = 0b0010001;
    font_monogram_data[49][ 5] = 0b0010001;
    font_monogram_data[49][ 6] = 0b0001111;
    font_monogram_data[49][ 7] = 0b0010001;
    font_monogram_data[49][ 8] = 0b0010001;
    font_monogram_data[49][ 9] = 0b0010001;
    font_monogram_chars[0x52] = 49;
    // 'S'
    font_monogram_data[50][ 3] = 0b0001110;
    font_monogram_data[50][ 4] = 0b0010001;
    font_monogram_data[50][ 5] = 0b0000001;
    font_monogram_data[50][ 6] = 0b0001110;
    font_monogram_data[50][ 7] = 0b0010000;
    font_monogram_data[50][ 8] = 0b0010001;
    font_monogram_data[50][ 9] = 0b0001110;
    font_monogram_chars[0x53] = 50;
    // 'T'
    font_monogram_data[51][ 3] = 0b0011111;
    font_monogram_data[51][ 4] = 0b0000100;
    font_monogram_data[51][ 5] = 0b0000100;
    font_monogram_data[51][ 6] = 0b0000100;
    font_monogram_data[51][ 7] = 0b0000100;
    font_monogram_data[51][ 8] = 0b0000100;
    font_monogram_data[51][ 9] = 0b0000100;
    font_monogram_chars[0x54] = 51;
    // 'U'
    font_monogram_data[52][ 3] = 0b0010001;
    font_monogram_data[52][ 4] = 0b0010001;
    font_monogram_data[52][ 5] = 0b0010001;
    font_monogram_data[52][ 6] = 0b0010001;
    font_monogram_data[52][ 7] = 0b0010001;
    font_monogram_data[52][ 8] = 0b0010001;
    font_monogram_data[52][ 9] = 0b0001110;
    font_monogram_chars[0x55] = 52;
    // 'V'
    font_monogram_data[53][ 3] = 0b0010001;
    font_monogram_data[53][ 4] = 0b0010001;
    font_monogram_data[53][ 5] = 0b0010001;
    font_monogram_data[53][ 6] = 0b0010001;
    font_monogram_data[53][ 7] = 0b0001010;
    font_monogram_data[53][ 8] = 0b0001010;
    font_monogram_data[53][ 9] = 0b0000100;
    font_monogram_chars[0x56] = 53;
    // 'W'
    font_monogram_data[54][ 3] = 0b0010001;
    font_monogram_data[54][ 4] = 0b0010001;
    font_monogram_data[54][ 5] = 0b0010001;
    font_monogram_data[54][ 6] = 0b0010001;
    font_monogram_data[54][ 7] = 0b0010101;
    font_monogram_data[54][ 8] = 0b0011011;
    font_monogram_data[54][ 9] = 0b0010001;
    font_monogram_chars[0x57] = 54;
    // 'X'
    font_monogram_data[55][ 3] = 0b0010001;
    font_monogram_data[55][ 4] = 0b0010001;
    font_monogram_data[55][ 5] = 0b0001010;
    font_monogram_data[55][ 6] = 0b0000100;
    font_monogram_data[55][ 7] = 0b0001010;
    font_monogram_data[55][ 8] = 0b0010001;
    font_monogram_data[55][ 9] = 0b0010001;
    font_monogram_chars[0x58] = 55;
    // 'Y'
    font_monogram_data[56][ 3] = 0b0010001;
    font_monogram_data[56][ 4] = 0b0010001;
    font_monogram_data[56][ 5] = 0b0001010;
    font_monogram_data[56][ 6] = 0b0000100;
    font_monogram_data[56][ 7] = 0b0000100;
    font_monogram_data[56][ 8] = 0b0000100;
    font_monogram_data[56][ 9] = 0b0000100;
    font_monogram_chars[0x59] = 56;
    // 'Z'
    font_monogram_data[57][ 3] = 0b0011111;
    font_monogram_data[57][ 4] = 0b0010000;
    font_monogram_data[57][ 5] = 0b0001000;
    font_monogram_data[57][ 6] = 0b0000100;
    font_monogram_data[57][ 7] = 0b0000010;
    font_monogram_data[57][ 8] = 0b0000001;
    font_monogram_data[57][ 9] = 0b0011111;
    font_monogram_chars[0x5a] = 57;
    // '['
    font_monogram_data[58][ 3] = 0b0001100;
    font_monogram_data[58][ 4] = 0b0000100;
    font_monogram_data[58][ 5] = 0b0000100;
    font_monogram_data[58][ 6] = 0b0000100;
    font_monogram_data[58][ 7] = 0b0000100;
    font_monogram_data[58][ 8] = 0b0000100;
    font_monogram_data[58][ 9] = 0b0001100;
    font_monogram_chars[0x5b] = 58;
    // '\\'
    font_monogram_data[59][ 3] = 0b0000001;
    font_monogram_data[59][ 4] = 0b0000001;
    font_monogram_data[59][ 5] = 0b0000010;
    font_monogram_data[59][ 6] = 0b0000100;
    font_monogram_data[59][ 7] = 0b0001000;
    font_monogram_data[59][ 8] = 0b0010000;
    font_monogram_data[59][ 9] = 0b0010000;
    font_monogram_chars[0x5c] = 59;
    // ']'
    font_monogram_data[60][ 3] = 0b0000110;
    font_monogram_data[60][ 4] = 0b0000100;
    font_monogram_data[60][ 5] = 0b0000100;
    font_monogram_data[60][ 6] = 0b0000100;
    font_monogram_data[60][ 7] = 0b0000100;
    font_monogram_data[60][ 8] = 0b0000100;
    font_monogram_data[60][ 9] = 0b0000110;
    font_monogram_chars[0x5d] = 60;
    // '^'
    font_monogram_data[61][ 3] = 0b0000100;
    font_monogram_data[61][ 4] = 0b0001010;
    font_monogram_data[61][ 5] = 0b0010001;
    font_monogram_chars[0x5e] = 61;
    // '_'
    font_monogram_data[62][ 9] = 0b0011111;
    font_monogram_chars[0x5f] = 62;
    // '`'
    font_monogram_data[63][ 3] = 0b0000010;
    font_monogram_data[63][ 4] = 0b0000100;
    font_monogram_chars[0x60] = 63;
    // 'a'
    font_monogram_data[64][ 5] = 0b0011110;
    font_monogram_data[64][ 6] = 0b0010001;
    font_monogram_data[64][ 7] = 0b0010001;
    font_monogram_data[64][ 8] = 0b0010001;
    font_monogram_data[64][ 9] = 0b0011110;
    font_monogram_chars[0x61] = 64;
    // 'b'
    font_monogram_data[65][ 3] = 0b0000001;
    font_monogram_data[65][ 4] = 0b0000001;
    font_monogram_data[65][ 5] = 0b0001111;
    font_monogram_data[65][ 6] = 0b0010001;
    font_monogram_data[65][ 7] = 0b0010001;
    font_monogram_data[65][ 8] = 0b0010001;
    font_monogram_data[65][ 9] = 0b0001111;
    font_monogram_chars[0x62] = 65;
    // 'c'
    font_monogram_data[66][ 5] = 0b0001110;
    font_monogram_data[66][ 6] = 0b0010001;
    font_monogram_data[66][ 7] = 0b0000001;
    font_monogram_data[66][ 8] = 0b0010001;
    font_monogram_data[66][ 9] = 0b0001110;
    font_monogram_chars[0x63] = 66;
    // 'd'
    font_monogram_data[67][ 3] = 0b0010000;
    font_monogram_data[67][ 4] = 0b0010000;
    font_monogram_data[67][ 5] = 0b0011110;
    font_monogram_data[67][ 6] = 0b0010001;
    font_monogram_data[67][ 7] = 0b0010001;
    font_monogram_data[67][ 8] = 0b0010001;
    font_monogram_data[67][ 9] = 0b0011110;
    font_monogram_chars[0x64] = 67;
    // 'e'
    font_monogram_data[68][ 5] = 0b0001110;
    font_monogram_data[68][ 6] = 0b0010001;
    font_monogram_data[68][ 7] = 0b0011111;
    font_monogram_data[68][ 8] = 0b0000001;
    font_monogram_data[68][ 9] = 0b0001110;
    font_monogram_chars[0x65] = 68;
    // 'f'
    font_monogram_data[69][ 3] = 0b0001100;
    font_monogram_data[69][ 4] = 0b0010010;
    font_monogram_data[69][ 5] = 0b0000010;
    font_monogram_data[69][ 6] = 0b0001111;
    font_monogram_data[69][ 7] = 0b0000010;
    font_monogram_data[69][ 8] = 0b0000010;
    font_monogram_data[69][ 9] = 0b0000010;
    font_monogram_chars[0x66] = 69;
    // 'g'
    font_monogram_data[70][ 5] = 0b0011110;
    font_monogram_data[70][ 6] = 0b0010001;
    font_monogram_data[70][ 7] = 0b0010001;
    font_monogram_data[70][ 8] = 0b0010001;
    font_monogram_data[70][ 9] = 0b0011110;
    font_monogram_data[70][10] = 0b0010000;
    font_monogram_data[70][11] = 0b0001110;
    font_monogram_chars[0x67] = 70;
    // 'h'
    font_monogram_data[71][ 3] = 0b0000001;
    font_monogram_data[71][ 4] = 0b0000001;
    font_monogram_data[71][ 5] = 0b0001111;
    font_monogram_data[71][ 6] = 0b0010001;
    font_monogram_data[71][ 7] = 0b0010001;
    font_monogram_data[71][ 8] = 0b0010001;
    font_monogram_data[71][ 9] = 0b0010001;
    font_monogram_chars[0x68] = 71;
    // 'i'
    font_monogram_data[72][ 3] = 0b0000100;
    font_monogram_data[72][ 5] = 0b0000110;
    font_monogram_data[72][ 6] = 0b0000100;
    font_monogram_data[72][ 7] = 0b0000100;
    font_monogram_data[72][ 8] = 0b0000100;
    font_monogram_data[72][ 9] = 0b0011111;
    font_monogram_chars[0x69] = 72;
    // 'j'
    font_monogram_data[73][ 3] = 0b0010000;
    font_monogram_data[73][ 5] = 0b0011000;
    font_monogram_data[73][ 6] = 0b0010000;
    font_monogram_data[73][ 7] = 0b0010000;
    font_monogram_data[73][ 8] = 0b0010000;
    font_monogram_data[73][ 9] = 0b0010000;
    font_monogram_data[73][10] = 0b0010001;
    font_monogram_data[73][11] = 0b0001110;
    font_monogram_chars[0x6a] = 73;
    // 'k'
    font_monogram_data[74][ 3] = 0b0000001;
    font_monogram_data[74][ 4] = 0b0000001;
    font_monogram_data[74][ 5] = 0b0010001;
    font_monogram_data[74][ 6] = 0b0001001;
    font_monogram_data[74][ 7] = 0b0000111;
    font_monogram_data[74][ 8] = 0b0001001;
    font_monogram_data[74][ 9] = 0b0010001;
    font_monogram_chars[0x6b] = 74;
    // 'l'
    font_monogram_data[75][ 3] = 0b0000011;
    font_monogram_data[75][ 4] = 0b0000010;
    font_monogram_data[75][ 5] = 0b0000010;
    font_monogram_data[75][ 6] = 0b0000010;
    font_monogram_data[75][ 7] = 0b0000010;
    font_monogram_data[75][ 8] = 0b0000010;
    font_monogram_data[75][ 9] = 0b0011100;
    font_monogram_chars[0x6c] = 75;
    // 'm'
    font_monogram_data[76][ 5] = 0b0001111;
    font_monogram_data[76][ 6] = 0b0010101;
    font_monogram_data[76][ 7] = 0b0010101;
    font_monogram_data[76][ 8] = 0b0010101;
    font_monogram_data[76][ 9] = 0b0010101;
    font_monogram_chars[0x6d] = 76;
    // 'n'
    font_monogram_data[77][ 5] = 0b0001111;
    font_monogram_data[77][ 6] = 0b0010001;
    font_monogram_data[77][ 7] = 0b0010001;
    font_monogram_data[77][ 8] = 0b0010001;
    font_monogram_data[77][ 9] = 0b0010001;
    font_monogram_chars[0x6e] = 77;
    // 'o'
    font_monogram_data[78][ 5] = 0b0001110;
    font_monogram_data[78][ 6] = 0b0010001;
    font_monogram_data[78][ 7] = 0b0010001;
    font_monogram_data[78][ 8] = 0b0010001;
    font_monogram_data[78][ 9] = 0b0001110;
    font_monogram_chars[0x6f] = 78;
    // 'p'
    font_monogram_data[79][ 5] = 0b0001111;
    font_monogram_data[79][ 6] = 0b0010001;
    font_monogram_data[79][ 7] = 0b0010001;
    font_monogram_data[79][ 8] = 0b0010001;
    font_monogram_data[79][ 9] = 0b0001111;
    font_monogram_data[79][10] = 0b0000001;
    font_monogram_data[79][11] = 0b0000001;
    font_monogram_chars[0x70] = 79;
    // 'q'
    font_monogram_data[80][ 5] = 0b0011110;
    font_monogram_data[80][ 6] = 0b0010001;
    font_monogram_data[80][ 7] = 0b0010001;
    font_monogram_data[80][ 8] = 0b0010001;
    font_monogram_data[80][ 9] = 0b0011110;
    font_monogram_data[80][10] = 0b0010000;
    font_monogram_data[80][11] = 0b0010000;
    font_monogram_chars[0x71] = 80;
    // 'r'
    font_monogram_data[81][ 5] = 0b0001101;
    font_monogram_data[81][ 6] = 0b0010011;
    font_monogram_data[81][ 7] = 0b0000001;
    font_monogram_data[81][ 8] = 0b0000001;
    font_monogram_data[81][ 9] = 0b0000001;
    font_monogram_chars[0x72] = 81;
    // 's'
    font_monogram_data[82][ 5] = 0b0011110;
    font_monogram_data[82][ 6] = 0b0000001;
    font_monogram_data[82][ 7] = 0b0001110;
    font_monogram_data[82][ 8] = 0b0010000;
    font_monogram_data[82][ 9] = 0b0001111;
    font_monogram_chars[0x73] = 82;
    // 't'
    font_monogram_data[83][ 3] = 0b0000010;
    font_monogram_data[83][ 4] = 0b0000010;
    font_monogram_data[83][ 5] = 0b0001111;
    font_monogram_data[83][ 6] = 0b0000010;
    font_monogram_data[83][ 7] = 0b0000010;
    font_monogram_data[83][ 8] = 0b0000010;
    font_monogram_data[83][ 9] = 0b0011100;
    font_monogram_chars[0x74] = 83;
    // 'u'
    font_monogram_data[84][ 5] = 0b0010001;
    font_monogram_data[84][ 6] = 0b0010001;
    font_monogram_data[84][ 7] = 0b0010001;
    font_monogram_data[84][ 8] = 0b0010001;
    font_monogram_data[84][ 9] = 0b0011110;
    font_monogram_chars[0x75] = 84;
    // 'v'
    font_monogram_data[85][ 5] = 0b0010001;
    font_monogram_data[85][ 6] = 0b0010001;
    font_monogram_data[85][ 7] = 0b0010001;
    font_monogram_data[85][ 8] = 0b0001010;
    font_monogram_data[85][ 9] = 0b0000100;
    font_monogram_chars[0x76] = 85;
    // 'w'
    font_monogram_data[86][ 5] = 0b0010001;
    font_monogram_data[86][ 6] = 0b0010001;
    font_monogram_data[86][ 7] = 0b0010101;
    font_monogram_data[86][ 8] = 0b0010101;
    font_monogram_data[86][ 9] = 0b0001010;
    font_monogram_chars[0x77] = 86;
    // 'x'
    font_monogram_data[87][ 5] = 0b0010001;
    font_monogram_data[87][ 6] = 0b0001010;
    font_monogram_data[87][ 7] = 0b0000100;
    font_monogram_data[87][ 8] = 0b0001010;
    font_monogram_data[87][ 9] = 0b0010001;
    font_monogram_chars[0x78] = 87;
    // 'y'
    font_monogram_data[88][ 5] = 0b0010001;
    font_monogram_data[88][ 6] = 0b0010001;
    font_monogram_data[88][ 7] = 0b0010001;
    font_monogram_data[88][ 8] = 0b0010001;
    font_monogram_data[88][ 9] = 0b0011110;
    font_monogram_data[88][10] = 0b0010000;
    font_monogram_data[88][11] = 0b0001110;
    font_monogram_chars[0x79] = 88;
    // 'z'
    font_monogram_data[89][ 5] = 0b0011111;
    font_monogram_data[89][ 6] = 0b0001000;
    font_monogram_data[89][ 7] = 0b0000100;
    font_monogram_data[89][ 8] = 0b0000010;
    font_monogram_data[89][ 9] = 0b0011111;
    font_monogram_chars[0x7a] = 89;
    // '{'
    font_monogram_data[90][ 3] = 0b0001000;
    font_monogram_data[90][ 4] = 0b0000100;
    font_monogram_data[90][ 5] = 0b0000100;
    font_monogram_data[90][ 6] = 0b0000010;
    font_monogram_data[90][ 7] = 0b0000100;
    font_monogram_data[90][ 8] = 0b0000100;
    font_monogram_data[90][ 9] = 0b0001000;
    font_monogram_chars[0x7b] = 90;
    // '|'
    font_monogram_data[91][ 3] = 0b0000100;
    font_monogram_data[91][ 4] = 0b0000100;
    font_monogram_data[91][ 5] = 0b0000100;
    font_monogram_data[91][ 6] = 0b0000100;
    font_monogram_data[91][ 7] = 0b0000100;
    font_monogram_data[91][ 8] = 0b0000100;
    font_monogram_data[91][ 9] = 0b0000100;
    font_monogram_chars[0x7c] = 91;
    // '}'
    font_monogram_data[92][ 3] = 0b0000010;
    font_monogram_data[92][ 4] = 0b0000100;
    font_monogram_data[92][ 5] = 0b0000100;
    font_monogram_data[92][ 6] = 0b0001000;
    font_monogram_data[92][ 7] = 0b0000100;
    font_monogram_data[92][ 8] = 0b0000100;
    font_monogram_data[92][ 9] = 0b0000010;
    font_monogram_chars[0x7d] = 92;
    // '~'
    font_monogram_data[93][ 5] = 0b0010010;
    font_monogram_data[93][ 6] = 0b0001101;
    font_monogram_chars[0x7e] = 93;
    // '¡'
    font_monogram_data[94][ 3] = 0b0000100;
    font_monogram_data[94][ 5] = 0b0000100;
    font_monogram_data[94][ 6] = 0b0000100;
    font_monogram_data[94][ 7] = 0b0000100;
    font_monogram_data[94][ 8] = 0b0000100;
    font_monogram_data[94][ 9] = 0b0000100;
    font_monogram_chars[0xa1] = 94;
    // '¢'
    font_monogram_data[95][ 3] = 0b0000100;
    font_monogram_data[95][ 4] = 0b0001110;
    font_monogram_data[95][ 5] = 0b0010101;
    font_monogram_data[95][ 6] = 0b0000101;
    font_monogram_data[95][ 7] = 0b0010101;
    font_monogram_data[95][ 8] = 0b0001110;
    font_monogram_data[95][ 9] = 0b0000100;
    font_monogram_chars[0xa2] = 95;
    // '£'
    font_monogram_data[96][ 3] = 0b0001100;
    font_monogram_data[96][ 4] = 0b0010010;
    font_monogram_data[96][ 5] = 0b0000010;
    font_monogram_data[96][ 6] = 0b0001111;
    font_monogram_data[96][ 7] = 0b0000010;
    font_monogram_data[96][ 8] = 0b0000010;
    font_monogram_data[96][ 9] = 0b0011111;
    font_monogram_chars[0xa3] = 96;
    // '¤'
    font_monogram_data[97][ 4] = 0b0010001;
    font_monogram_data[97][ 5] = 0b0001110;
    font_monogram_data[97][ 6] = 0b0001010;
    font_monogram_data[97][ 7] = 0b0001110;
    font_monogram_data[97][ 8] = 0b0010001;
    font_monogram_chars[0xa4] = 97;
    // '¥'
    font_monogram_data[98][ 3] = 0b0010001;
    font_monogram_data[98][ 4] = 0b0001010;
    font_monogram_data[98][ 5] = 0b0000100;
    font_monogram_data[98][ 6] = 0b0011111;
    font_monogram_data[98][ 7] = 0b0000100;
    font_monogram_data[98][ 8] = 0b0011111;
    font_monogram_data[98][ 9] = 0b0000100;
    font_monogram_chars[0xa5] = 98;
    // '¦'
    font_monogram_data[99][ 3] = 0b0000100;
    font_monogram_data[99][ 4] = 0b0000100;
    font_monogram_data[99][ 5] = 0b0000100;
    font_monogram_data[99][ 7] = 0b0000100;
    font_monogram_data[99][ 8] = 0b0000100;
    font_monogram_data[99][ 9] = 0b0000100;
    font_monogram_chars[0xa6] = 99;
    // '§'
    font_monogram_data[100][ 3] = 0b0011110;
    font_monogram_data[100][ 4] = 0b0000001;
    font_monogram_data[100][ 5] = 0b0001110;
    font_monogram_data[100][ 6] = 0b0010001;
    font_monogram_data[100][ 7] = 0b0001110;
    font_monogram_data[100][ 8] = 0b0010000;
    font_monogram_data[100][ 9] = 0b0001111;
    font_monogram_chars[0xa7] = 100;
    // '¨'
    font_monogram_data[101][ 3] = 0b0001010;
    font_monogram_chars[0xa8] = 101;
    // '©'
    font_monogram_data[102][ 3] = 0b0001110;
    font_monogram_data[102][ 4] = 0b0011011;
    font_monogram_data[102][ 5] = 0b0010101;
    font_monogram_data[102][ 6] = 0b0011101;
    font_monogram_data[102][ 7] = 0b0010101;
    font_monogram_data[102][ 8] = 0b0011011;
    font_monogram_data[102][ 9] = 0b0001110;
    font_monogram_chars[0xa9] = 102;
    // 'ª'
    font_monogram_data[103][ 3] = 0b0001110;
    font_monogram_data[103][ 4] = 0b0001001;
    font_monogram_data[103][ 5] = 0b0001001;
    font_monogram_data[103][ 6] = 0b0001001;
    font_monogram_data[103][ 7] = 0b0001110;
    font_monogram_chars[0xaa] = 103;
    // '«'
    font_monogram_data[104][ 5] = 0b0010010;
    font_monogram_data[104][ 6] = 0b0001001;
    font_monogram_data[104][ 7] = 0b0010010;
    font_monogram_chars[0xab] = 104;
    // '¬'
    font_monogram_data[105][ 6] = 0b0011111;
    font_monogram_data[105][ 7] = 0b0010000;
    font_monogram_chars[0xac] = 105;
    // '®'
    font_monogram_data[106][ 3] = 0b0001110;
    font_monogram_data[106][ 4] = 0b0011001;
    font_monogram_data[106][ 5] = 0b0010101;
    font_monogram_data[106][ 6] = 0b0010101;
    font_monogram_data[106][ 7] = 0b0011001;
    font_monogram_data[106][ 8] = 0b0010101;
    font_monogram_data[106][ 9] = 0b0001110;
    font_monogram_chars[0xae] = 106;
    // '¯'
    font_monogram_data[107][ 6] = 0b0011111;
    font_monogram_chars[0xaf] = 107;
    // '°'
    font_monogram_data[108][ 3] = 0b0000110;
    font_monogram_data[108][ 4] = 0b0001001;
    font_monogram_data[108][ 5] = 0b0001001;
    font_monogram_data[108][ 6] = 0b0000110;
    font_monogram_chars[0xb0] = 108;
    // '±'
    font_monogram_data[109][ 3] = 0b0000100;
    font_monogram_data[109][ 4] = 0b0000100;
    font_monogram_data[109][ 5] = 0b0011111;
    font_monogram_data[109][ 6] = 0b0000100;
    font_monogram_data[109][ 7] = 0b0000100;
    font_monogram_data[109][ 9] = 0b0011111;
    font_monogram_chars[0xb1] = 109;
    // '²'
    font_monogram_data[110][ 3] = 0b0000011;
    font_monogram_data[110][ 4] = 0b0000100;
    font_monogram_data[110][ 5] = 0b0000010;
    font_monogram_data[110][ 6] = 0b0000001;
    font_monogram_data[110][ 7] = 0b0000111;
    font_monogram_chars[0xb2] = 110;
    // '³'
    font_monogram_data[111][ 3] = 0b0000011;
    font_monogram_data[111][ 4] = 0b0000100;
    font_monogram_data[111][ 5] = 0b0000010;
    font_monogram_data[111][ 6] = 0b0000100;
    font_monogram_data[111][ 7] = 0b0000011;
    font_monogram_chars[0xb3] = 111;
    // '´'
    font_monogram_data[112][ 2] = 0b0001000;
    font_monogram_data[112][ 3] = 0b0000100;
    font_monogram_chars[0xb4] = 112;
    // 'µ'
    font_monogram_data[113][ 5] = 0b0010001;
    font_monogram_data[113][ 6] = 0b0010001;
    font_monogram_data[113][ 7] = 0b0010001;
    font_monogram_data[113][ 8] = 0b0010001;
    font_monogram_data[113][ 9] = 0b0001111;
    font_monogram_data[113][10] = 0b0000001;
    font_monogram_data[113][11] = 0b0000001;
    font_monogram_chars[0xb5] = 113;
    // '¶'
    font_monogram_data[114][ 3] = 0b0011110;
    font_monogram_data[114][ 4] = 0b0010111;
    font_monogram_data[114][ 5] = 0b0010111;
    font_monogram_data[114][ 6] = 0b0010111;
    font_monogram_data[114][ 7] = 0b0010110;
    font_monogram_data[114][ 8] = 0b0010100;
    font_monogram_data[114][ 9] = 0b0010100;
    font_monogram_chars[0xb6] = 114;
    // '·'
    font_monogram_data[115][ 3] = 0b0000100;
    font_monogram_chars[0xb7] = 115;
    // '¸'
    font_monogram_data[116][ 9] = 0b0000100;
    font_monogram_data[116][10] = 0b0001000;
    font_monogram_data[116][11] = 0b0000110;
    font_monogram_chars[0xb8] = 116;
    // '¹'
    font_monogram_data[117][ 3] = 0b0000010;
    font_monogram_data[117][ 4] = 0b0000011;
    font_monogram_data[117][ 5] = 0b0000010;
    font_monogram_data[117][ 6] = 0b0000010;
    font_monogram_data[117][ 7] = 0b0000111;
    font_monogram_chars[0xb9] = 117;
    // 'º'
    font_monogram_data[118][ 3] = 0b0000110;
    font_monogram_data[118][ 4] = 0b0001001;
    font_monogram_data[118][ 5] = 0b0001001;
    font_monogram_data[118][ 6] = 0b0001001;
    font_monogram_data[118][ 7] = 0b0000110;
    font_monogram_chars[0xba] = 118;
    // '»'
    font_monogram_data[119][ 5] = 0b0001001;
    font_monogram_data[119][ 6] = 0b0010010;
    font_monogram_data[119][ 7] = 0b0001001;
    font_monogram_chars[0xbb] = 119;
    // '¼'
    font_monogram_data[120][ 3] = 0b0000001;
    font_monogram_data[120][ 4] = 0b0001001;
    font_monogram_data[120][ 5] = 0b0000101;
    font_monogram_data[120][ 6] = 0b0000010;
    font_monogram_data[120][ 7] = 0b0010101;
    font_monogram_data[120][ 8] = 0b0011100;
    font_monogram_data[120][ 9] = 0b0010000;
    font_monogram_chars[0xbc] = 120;
    // '½'
    font_monogram_data[121][ 3] = 0b0000001;
    font_monogram_data[121][ 4] = 0b0001001;
    font_monogram_data[121][ 5] = 0b0000101;
    font_monogram_data[121][ 6] = 0b0001110;
    font_monogram_data[121][ 7] = 0b0010001;
    font_monogram_data[121][ 8] = 0b0001000;
    font_monogram_data[121][ 9] = 0b0011100;
    font_monogram_chars[0xbd] = 121;
    // '¾'
    font_monogram_data[122][ 3] = 0b0000111;
    font_monogram_data[122][ 4] = 0b0010110;
    font_monogram_data[122][ 5] = 0b0001111;
    font_monogram_data[122][ 6] = 0b0000100;
    font_monogram_data[122][ 7] = 0b0010110;
    font_monogram_data[122][ 8] = 0b0011101;
    font_monogram_data[122][ 9] = 0b0010000;
    font_monogram_chars[0xbe] = 122;
    // '¿'
    font_monogram_data[123][ 3] = 0b0000100;
    font_monogram_data[123][ 5] = 0b0000100;
    font_monogram_data[123][ 6] = 0b0000010;
    font_monogram_data[123][ 7] = 0b0000001;
    font_monogram_data[123][ 8] = 0b0010001;
    font_monogram_data[123][ 9] = 0b0001110;
    font_monogram_chars[0xbf] = 123;
    // 'À'
    font_monogram_data[124][ 0] = 0b0000010;
    font_monogram_data[124][ 1] = 0b0000100;
    font_monogram_data[124][ 3] = 0b0001110;
    font_monogram_data[124][ 4] = 0b0010001;
    font_monogram_data[124][ 5] = 0b0010001;
    font_monogram_data[124][ 6] = 0b0011111;
    font_monogram_data[124][ 7] = 0b0010001;
    font_monogram_data[124][ 8] = 0b0010001;
    font_monogram_data[124][ 9] = 0b0010001;
    font_monogram_chars[0xc0] = 124;
    // 'Á'
    font_monogram_data[125][ 0] = 0b0001000;
    font_monogram_data[125][ 1] = 0b0000100;
    font_monogram_data[125][ 3] = 0b0001110;
    font_monogram_data[125][ 4] = 0b0010001;
    font_monogram_data[125][ 5] = 0b0010001;
    font_monogram_data[125][ 6] = 0b0011111;
    font_monogram_data[125][ 7] = 0b0010001;
    font_monogram_data[125][ 8] = 0b0010001;
    font_monogram_data[125][ 9] = 0b0010001;
    font_monogram_chars[0xc1] = 125;
    // 'Â'
    font_monogram_data[126][ 0] = 0b0000100;
    font_monogram_data[126][ 1] = 0b0001010;
    font_monogram_data[126][ 3] = 0b0001110;
    font_monogram_data[126][ 4] = 0b0010001;
    font_monogram_data[126][ 5] = 0b0010001;
    font_monogram_data[126][ 6] = 0b0011111;
    font_monogram_data[126][ 7] = 0b0010001;
    font_monogram_data[126][ 8] = 0b0010001;
    font_monogram_data[126][ 9] = 0b0010001;
    font_monogram_chars[0xc2] = 126;
    // 'Ã'
    font_monogram_data[127][ 0] = 0b0010110;
    font_monogram_data[127][ 1] = 0b0001001;
    font_monogram_data[127][ 3] = 0b0001110;
    font_monogram_data[127][ 4] = 0b0010001;
    font_monogram_data[127][ 5] = 0b0010001;
    font_monogram_data[127][ 6] = 0b0011111;
    font_monogram_data[127][ 7] = 0b0010001;
    font_monogram_data[127][ 8] = 0b0010001;
    font_monogram_data[127][ 9] = 0b0010001;
    font_monogram_chars[0xc3] = 127;
    // 'Ä'
    font_monogram_data[128][ 1] = 0b0001010;
    font_monogram_data[128][ 3] = 0b0001110;
    font_monogram_data[128][ 4] = 0b0010001;
    font_monogram_data[128][ 5] = 0b0010001;
    font_monogram_data[128][ 6] = 0b0011111;
    font_monogram_data[128][ 7] = 0b0010001;
    font_monogram_data[128][ 8] = 0b0010001;
    font_monogram_data[128][ 9] = 0b0010001;
    font_monogram_chars[0xc4] = 128;
    // 'Å'
    font_monogram_data[129][ 0] = 0b0000100;
    font_monogram_data[129][ 1] = 0b0001010;
    font_monogram_data[129][ 2] = 0b0000100;
    font_monogram_data[129][ 3] = 0b0001110;
    font_monogram_data[129][ 4] = 0b0010001;
    font_monogram_data[129][ 5] = 0b0010001;
    font_monogram_data[129][ 6] = 0b0011111;
    font_monogram_data[129][ 7] = 0b0010001;
    font_monogram_data[129][ 8] = 0b0010001;
    font_monogram_data[129][ 9] = 0b0010001;
    font_monogram_chars[0xc5] = 129;
    // 'Æ'
    font_monogram_data[130][ 3] = 0b0011110;
    font_monogram_data[130][ 4] = 0b0000101;
    font_monogram_data[130][ 5] = 0b0000101;
    font_monogram_data[130][ 6] = 0b0011111;
    font_monogram_data[130][ 7] = 0b0000101;
    font_monogram_data[130][ 8] = 0b0000101;
    font_monogram_data[130][ 9] = 0b0011101;
    font_monogram_chars[0xc6] = 130;
    // 'Ç'
    font_monogram_data[131][ 3] = 0b0001110;
    font_monogram_data[131][ 4] = 0b0010001;
    font_monogram_data[131][ 5] = 0b0000001;
    font_monogram_data[131][ 6] = 0b0000001;
    font_monogram_data[131][ 7] = 0b0000001;
    font_monogram_data[131][ 8] = 0b0010001;
    font_monogram_data[131][ 9] = 0b0001110;
    font_monogram_data[131][10] = 0b0001000;
    font_monogram_data[131][11] = 0b0000110;
    font_monogram_chars[0xc7] = 131;
    // 'È'
    font_monogram_data[132][ 0] = 0b0000010;
    font_monogram_data[132][ 1] = 0b0000100;
    font_monogram_data[132][ 3] = 0b0011111;
    font_monogram_data[132][ 4] = 0b0000001;
    font_monogram_data[132][ 5] = 0b0000001;
    font_monogram_data[132][ 6] = 0b0001111;
    font_monogram_data[132][ 7] = 0b0000001;
    font_monogram_data[132][ 8] = 0b0000001;
    font_monogram_data[132][ 9] = 0b0011111;
    font_monogram_chars[0xc8] = 132;
    // 'É'
    font_monogram_data[133][ 0] = 0b0001000;
    font_monogram_data[133][ 1] = 0b0000100;
    font_monogram_data[133][ 3] = 0b0011111;
    font_monogram_data[133][ 4] = 0b0000001;
    font_monogram_data[133][ 5] = 0b0000001;
    font_monogram_data[133][ 6] = 0b0001111;
    font_monogram_data[133][ 7] = 0b0000001;
    font_monogram_data[133][ 8] = 0b0000001;
    font_monogram_data[133][ 9] = 0b0011111;
    font_monogram_chars[0xc9] = 133;
    // 'Ê'
    font_monogram_data[134][ 0] = 0b0000100;
    font_monogram_data[134][ 1] = 0b0001010;
    font_monogram_data[134][ 3] = 0b0011111;
    font_monogram_data[134][ 4] = 0b0000001;
    font_monogram_data[134][ 5] = 0b0000001;
    font_monogram_data[134][ 6] = 0b0001111;
    font_monogram_data[134][ 7] = 0b0000001;
    font_monogram_data[134][ 8] = 0b0000001;
    font_monogram_data[134][ 9] = 0b0011111;
    font_monogram_chars[0xca] = 134;
    // 'Ë'
    font_monogram_data[135][ 1] = 0b0001010;
    font_monogram_data[135][ 3] = 0b0011111;
    font_monogram_data[135][ 4] = 0b0000001;
    font_monogram_data[135][ 5] = 0b0000001;
    font_monogram_data[135][ 6] = 0b0001111;
    font_monogram_data[135][ 7] = 0b0000001;
    font_monogram_data[135][ 8] = 0b0000001;
    font_monogram_data[135][ 9] = 0b0011111;
    font_monogram_chars[0xcb] = 135;
    // 'Ì'
    font_monogram_data[136][ 0] = 0b0000010;
    font_monogram_data[136][ 1] = 0b0000100;
    font_monogram_data[136][ 3] = 0b0011111;
    font_monogram_data[136][ 4] = 0b0000100;
    font_monogram_data[136][ 5] = 0b0000100;
    font_monogram_data[136][ 6] = 0b0000100;
    font_monogram_data[136][ 7] = 0b0000100;
    font_monogram_data[136][ 8] = 0b0000100;
    font_monogram_data[136][ 9] = 0b0011111;
    font_monogram_chars[0xcc] = 136;
    // 'Í'
    font_monogram_data[137][ 0] = 0b0001000;
    font_monogram_data[137][ 1] = 0b0000100;
    font_monogram_data[137][ 3] = 0b0011111;
    font_monogram_data[137][ 4] = 0b0000100;
    font_monogram_data[137][ 5] = 0b0000100;
    font_monogram_data[137][ 6] = 0b0000100;
    font_monogram_data[137][ 7] = 0b0000100;
    font_monogram_data[137][ 8] = 0b0000100;
    font_monogram_data[137][ 9] = 0b0011111;
    font_monogram_chars[0xcd] = 137;
    // 'Î'
    font_monogram_data[138][ 0] = 0b0000100;
    font_monogram_data[138][ 1] = 0b0001010;
    font_monogram_data[138][ 3] = 0b0011111;
    font_monogram_data[138][ 4] = 0b0000100;
    font_monogram_data[138][ 5] = 0b0000100;
    font_monogram_data[138][ 6] = 0b0000100;
    font_monogram_data[138][ 7] = 0b0000100;
    font_monogram_data[138][ 8] = 0b0000100;
    font_monogram_data[138][ 9] = 0b0011111;
    font_monogram_chars[0xce] = 138;
    // 'Ï'
    font_monogram_data[139][ 1] = 0b0001010;
    font_monogram_data[139][ 3] = 0b0011111;
    font_monogram_data[139][ 4] = 0b0000100;
    font_monogram_data[139][ 5] = 0b0000100;
    font_monogram_data[139][ 6] = 0b0000100;
    font_monogram_data[139][ 7] = 0b0000100;
    font_monogram_data[139][ 8] = 0b0000100;
    font_monogram_data[139][ 9] = 0b0011111;
    font_monogram_chars[0xcf] = 139;
    // 'Ð'
    font_monogram_data[140][ 3] = 0b0001111;
    font_monogram_data[140][ 4] = 0b0010001;
    font_monogram_data[140][ 5] = 0b0010001;
    font_monogram_data[140][ 6] = 0b0010011;
    font_monogram_data[140][ 7] = 0b0010001;
    font_monogram_data[140][ 8] = 0b0010001;
    font_monogram_data[140][ 9] = 0b0001111;
    font_monogram_chars[0xd0] = 140;
    // 'Ñ'
    font_monogram_data[141][ 0] = 0b0010110;
    font_monogram_data[141][ 1] = 0b0001001;
    font_monogram_data[141][ 3] = 0b0010001;
    font_monogram_data[141][ 4] = 0b0010001;
    font_monogram_data[141][ 5] = 0b0010011;
    font_monogram_data[141][ 6] = 0b0010101;
    font_monogram_data[141][ 7] = 0b0011001;
    font_monogram_data[141][ 8] = 0b0010001;
    font_monogram_data[141][ 9] = 0b0010001;
    font_monogram_chars[0xd1] = 141;
    // 'Ò'
    font_monogram_data[142][ 0] = 0b0000010;
    font_monogram_data[142][ 1] = 0b0000100;
    font_monogram_data[142][ 3] = 0b0001110;
    font_monogram_data[142][ 4] = 0b0010001;
    font_monogram_data[142][ 5] = 0b0010001;
    font_monogram_data[142][ 6] = 0b0010001;
    font_monogram_data[142][ 7] = 0b0010001;
    font_monogram_data[142][ 8] = 0b0010001;
    font_monogram_data[142][ 9] = 0b0001110;
    font_monogram_chars[0xd2] = 142;
    // 'Ó'
    font_monogram_data[143][ 0] = 0b0001000;
    font_monogram_data[143][ 1] = 0b0000100;
    font_monogram_data[143][ 3] = 0b0001110;
    font_monogram_data[143][ 4] = 0b0010001;
    font_monogram_data[143][ 5] = 0b0010001;
    font_monogram_data[143][ 6] = 0b0010001;
    font_monogram_data[143][ 7] = 0b0010001;
    font_monogram_data[143][ 8] = 0b0010001;
    font_monogram_data[143][ 9] = 0b0001110;
    font_monogram_chars[0xd3] = 143;
    // 'Ô'
    font_monogram_data[144][ 0] = 0b0000100;
    font_monogram_data[144][ 1] = 0b0001010;
    font_monogram_data[144][ 3] = 0b0001110;
    font_monogram_data[144][ 4] = 0b0010001;
    font_monogram_data[144][ 5] = 0b0010001;
    font_monogram_data[144][ 6] = 0b0010001;
    font_monogram_data[144][ 7] = 0b0010001;
    font_monogram_data[144][ 8] = 0b0010001;
    font_monogram_data[144][ 9] = 0b0001110;
    font_monogram_chars[0xd4] = 144;
    // 'Õ'
    font_monogram_data[145][ 0] = 0b0010110;
    font_monogram_data[145][ 1] = 0b0001001;
    font_monogram_data[145][ 3] = 0b0001110;
    font_monogram_data[145][ 4] = 0b0010001;
    font_monogram_data[145][ 5] = 0b0010001;
    font_monogram_data[145][ 6] = 0b0010001;
    font_monogram_data[145][ 7] = 0b0010001;
    font_monogram_data[145][ 8] = 0b0010001;
    font_monogram_data[145][ 9] = 0b0001110;
    font_monogram_chars[0xd5] = 145;
    // 'Ö'
    font_monogram_data[146][ 1] = 0b0001010;
    font_monogram_data[146][ 3] = 0b0001110;
    font_monogram_data[146][ 4] = 0b0010001;
    font_monogram_data[146][ 5] = 0b0010001;
    font_monogram_data[146][ 6] = 0b0010001;
    font_monogram_data[146][ 7] = 0b0010001;
    font_monogram_data[146][ 8] = 0b0010001;
    font_monogram_data[146][ 9] = 0b0001110;
    font_monogram_chars[0xd6] = 146;
    // '×'
    font_monogram_data[147][ 6] = 0b0001010;
    font_monogram_data[147][ 7] = 0b0000100;
    font_monogram_data[147][ 8] = 0b0001010;
    font_monogram_chars[0xd7] = 147;
    // 'Ø'
    font_monogram_data[148][ 3] = 0b0010110;
    font_monogram_data[148][ 4] = 0b0001001;
    font_monogram_data[148][ 5] = 0b0011001;
    font_monogram_data[148][ 6] = 0b0010101;
    font_monogram_data[148][ 7] = 0b0010011;
    font_monogram_data[148][ 8] = 0b0010010;
    font_monogram_data[148][ 9] = 0b0001101;
    font_monogram_chars[0xd8] = 148;
    // 'Ù'
    font_monogram_data[149][ 0] = 0b0000010;
    font_monogram_data[149][ 1] = 0b0000100;
    font_monogram_data[149][ 3] = 0b0010001;
    font_monogram_data[149][ 4] = 0b0010001;
    font_monogram_data[149][ 5] = 0b0010001;
    font_monogram_data[149][ 6] = 0b0010001;
    font_monogram_data[149][ 7] = 0b0010001;
    font_monogram_data[149][ 8] = 0b0010001;
    font_monogram_data[149][ 9] = 0b0001110;
    font_monogram_chars[0xd9] = 149;
    // 'Ú'
    font_monogram_data[150][ 0] = 0b0001000;
    font_monogram_data[150][ 1] = 0b0000100;
    font_monogram_data[150][ 3] = 0b0010001;
    font_monogram_data[150][ 4] = 0b0010001;
    font_monogram_data[150][ 5] = 0b0010001;
    font_monogram_data[150][ 6] = 0b0010001;
    font_monogram_data[150][ 7] = 0b0010001;
    font_monogram_data[150][ 8] = 0b0010001;
    font_monogram_data[150][ 9] = 0b0001110;
    font_monogram_chars[0xda] = 150;
    // 'Û'
    font_monogram_data[151][ 0] = 0b0000100;
    font_monogram_data[151][ 1] = 0b0001010;
    font_monogram_data[151][ 3] = 0b0010001;
    font_monogram_data[151][ 4] = 0b0010001;
    font_monogram_data[151][ 5] = 0b0010001;
    font_monogram_data[151][ 6] = 0b0010001;
    font_monogram_data[151][ 7] = 0b0010001;
    font_monogram_data[151][ 8] = 0b0010001;
    font_monogram_data[151][ 9] = 0b0001110;
    font_monogram_chars[0xdb] = 151;
    // 'Ü'
    font_monogram_data[152][ 1] = 0b0001010;
    font_monogram_data[152][ 3] = 0b0010001;
    font_monogram_data[152][ 4] = 0b0010001;
    font_monogram_data[152][ 5] = 0b0010001;
    font_monogram_data[152][ 6] = 0b0010001;
    font_monogram_data[152][ 7] = 0b0010001;
    font_monogram_data[152][ 8] = 0b0010001;
    font_monogram_data[152][ 9] = 0b0001110;
    font_monogram_chars[0xdc] = 152;
    // 'Ý'
    font_monogram_data[153][ 0] = 0b0001000;
    font_monogram_data[153][ 1] = 0b0000100;
    font_monogram_data[153][ 3] = 0b0010001;
    font_monogram_data[153][ 4] = 0b0010001;
    font_monogram_data[153][ 5] = 0b0001010;
    font_monogram_data[153][ 6] = 0b0000100;
    font_monogram_data[153][ 7] = 0b0000100;
    font_monogram_data[153][ 8] = 0b0000100;
    font_monogram_data[153][ 9] = 0b0000100;
    font_monogram_chars[0xdd] = 153;
    // 'Þ'
    font_monogram_data[154][ 3] = 0b0000001;
    font_monogram_data[154][ 4] = 0b0001111;
    font_monogram_data[154][ 5] = 0b0010001;
    font_monogram_data[154][ 6] = 0b0010001;
    font_monogram_data[154][ 7] = 0b0010001;
    font_monogram_data[154][ 8] = 0b0001111;
    font_monogram_data[154][ 9] = 0b0000001;
    font_monogram_chars[0xde] = 154;
    // 'ß'
    font_monogram_data[155][ 3] = 0b0000110;
    font_monogram_data[155][ 4] = 0b0001001;
    font_monogram_data[155][ 5] = 0b0001001;
    font_monogram_data[155][ 6] = 0b0001101;
    font_monogram_data[155][ 7] = 0b0010001;
    font_monogram_data[155][ 8] = 0b0010001;
    font_monogram_data[155][ 9] = 0b0001101;
    font_monogram_chars[0xdf] = 155;
    // 'à'
    font_monogram_data[156][ 2] = 0b0000010;
    font_monogram_data[156][ 3] = 0b0000100;
    font_monogram_data[156][ 5] = 0b0011110;
    font_monogram_data[156][ 6] = 0b0010001;
    font_monogram_data[156][ 7] = 0b0010001;
    font_monogram_data[156][ 8] = 0b0010001;
    font_monogram_data[156][ 9] = 0b0011110;
    font_monogram_chars[0xe0] = 156;
    // 'á'
    font_monogram_data[157][ 2] = 0b0001000;
    font_monogram_data[157][ 3] = 0b0000100;
    font_monogram_data[157][ 5] = 0b0011110;
    font_monogram_data[157][ 6] = 0b0010001;
    font_monogram_data[157][ 7] = 0b0010001;
    font_monogram_data[157][ 8] = 0b0010001;
    font_monogram_data[157][ 9] = 0b0011110;
    font_monogram_chars[0xe1] = 157;
    // 'â'
    font_monogram_data[158][ 2] = 0b0000100;
    font_monogram_data[158][ 3] = 0b0001010;
    font_monogram_data[158][ 5] = 0b0011110;
    font_monogram_data[158][ 6] = 0b0010001;
    font_monogram_data[158][ 7] = 0b0010001;
    font_monogram_data[158][ 8] = 0b0010001;
    font_monogram_data[158][ 9] = 0b0011110;
    font_monogram_chars[0xe2] = 158;
    // 'ã'
    font_monogram_data[159][ 2] = 0b0010110;
    font_monogram_data[159][ 3] = 0b0001001;
    font_monogram_data[159][ 5] = 0b0011110;
    font_monogram_data[159][ 6] = 0b0010001;
    font_monogram_data[159][ 7] = 0b0010001;
    font_monogram_data[159][ 8] = 0b0010001;
    font_monogram_data[159][ 9] = 0b0011110;
    font_monogram_chars[0xe3] = 159;
    // 'ä'
    font_monogram_data[160][ 3] = 0b0001010;
    font_monogram_data[160][ 5] = 0b0011110;
    font_monogram_data[160][ 6] = 0b0010001;
    font_monogram_data[160][ 7] = 0b0010001;
    font_monogram_data[160][ 8] = 0b0010001;
    font_monogram_data[160][ 9] = 0b0011110;
    font_monogram_chars[0xe4] = 160;
    // 'å'
    font_monogram_data[161][ 1] = 0b0000100;
    font_monogram_data[161][ 2] = 0b0001010;
    font_monogram_data[161][ 3] = 0b0000100;
    font_monogram_data[161][ 5] = 0b0011110;
    font_monogram_data[161][ 6] = 0b0010001;
    font_monogram_data[161][ 7] = 0b0010001;
    font_monogram_data[161][ 8] = 0b0010001;
    font_monogram_data[161][ 9] = 0b0011110;
    font_monogram_chars[0xe5] = 161;
    // 'æ'
    font_monogram_data[162][ 5] = 0b0001110;
    font_monogram_data[162][ 6] = 0b0010101;
    font_monogram_data[162][ 7] = 0b0011101;
    font_monogram_data[162][ 8] = 0b0000101;
    font_monogram_data[162][ 9] = 0b0011110;
    font_monogram_chars[0xe6] = 162;
    // 'ç'
    font_monogram_data[163][ 5] = 0b0001110;
    font_monogram_data[163][ 6] = 0b0010001;
    font_monogram_data[163][ 7] = 0b0000001;
    font_monogram_data[163][ 8] = 0b0010001;
    font_monogram_data[163][ 9] = 0b0001110;
    font_monogram_data[163][10] = 0b0001000;
    font_monogram_data[163][11] = 0b0000110;
    font_monogram_chars[0xe7] = 163;
    // 'è'
    font_monogram_data[164][ 2] = 0b0000010;
    font_monogram_data[164][ 3] = 0b0000100;
    font_monogram_data[164][ 5] = 0b0001110;
    font_monogram_data[164][ 6] = 0b0010001;
    font_monogram_data[164][ 7] = 0b0011111;
    font_monogram_data[164][ 8] = 0b0000001;
    font_monogram_data[164][ 9] = 0b0001110;
    font_monogram_chars[0xe8] = 164;
    // 'é'
    font_monogram_data[165][ 2] = 0b0001000;
    font_monogram_data[165][ 3] = 0b0000100;
    font_monogram_data[165][ 5] = 0b0001110;
    font_monogram_data[165][ 6] = 0b0010001;
    font_monogram_data[165][ 7] = 0b0011111;
    font_monogram_data[165][ 8] = 0b0000001;
    font_monogram_data[165][ 9] = 0b0001110;
    font_monogram_chars[0xe9] = 165;
    // 'ê'
    font_monogram_data[166][ 2] = 0b0000100;
    font_monogram_data[166][ 3] = 0b0001010;
    font_monogram_data[166][ 5] = 0b0001110;
    font_monogram_data[166][ 6] = 0b0010001;
    font_monogram_data[166][ 7] = 0b0011111;
    font_monogram_data[166][ 8] = 0b0000001;
    font_monogram_data[166][ 9] = 0b0001110;
    font_monogram_chars[0xea] = 166;
    // 'ë'
    font_monogram_data[167][ 3] = 0b0001010;
    font_monogram_data[167][ 5] = 0b0001110;
    font_monogram_data[167][ 6] = 0b0010001;
    font_monogram_data[167][ 7] = 0b0011111;
    font_monogram_data[167][ 8] = 0b0000001;
    font_monogram_data[167][ 9] = 0b0001110;
    font_monogram_chars[0xeb] = 167;
    // 'ì'
    font_monogram_data[168][ 2] = 0b0000010;
    font_monogram_data[168][ 3] = 0b0000100;
    font_monogram_data[168][ 5] = 0b0000110;
    font_monogram_data[168][ 6] = 0b0000100;
    font_monogram_data[168][ 7] = 0b0000100;
    font_monogram_data[168][ 8] = 0b0000100;
    font_monogram_data[168][ 9] = 0b0011111;
    font_monogram_chars[0xec] = 168;
    // 'í'
    font_monogram_data[169][ 2] = 0b0001000;
    font_monogram_data[169][ 3] = 0b0000100;
    font_monogram_data[169][ 5] = 0b0000110;
    font_monogram_data[169][ 6] = 0b0000100;
    font_monogram_data[169][ 7] = 0b0000100;
    font_monogram_data[169][ 8] = 0b0000100;
    font_monogram_data[169][ 9] = 0b0011111;
    font_monogram_chars[0xed] = 169;
    // 'î'
    font_monogram_data[170][ 2] = 0b0000100;
    font_monogram_data[170][ 3] = 0b0001010;
    font_monogram_data[170][ 5] = 0b0000110;
    font_monogram_data[170][ 6] = 0b0000100;
    font_monogram_data[170][ 7] = 0b0000100;
    font_monogram_data[170][ 8] = 0b0000100;
    font_monogram_data[170][ 9] = 0b0011111;
    font_monogram_chars[0xee] = 170;
    // 'ï'
    font_monogram_data[171][ 3] = 0b0001010;
    font_monogram_data[171][ 5] = 0b0000110;
    font_monogram_data[171][ 6] = 0b0000100;
    font_monogram_data[171][ 7] = 0b0000100;
    font_monogram_data[171][ 8] = 0b0000100;
    font_monogram_data[171][ 9] = 0b0011111;
    font_monogram_chars[0xef] = 171;
    // 'ð'
    font_monogram_data[172][ 2] = 0b0001110;
    font_monogram_data[172][ 3] = 0b0110000;
    font_monogram_data[172][ 4] = 0b0011000;
    font_monogram_data[172][ 5] = 0b0011110;
    font_monogram_data[172][ 6] = 0b0010001;
    font_monogram_data[172][ 7] = 0b0010001;
    font_monogram_data[172][ 8] = 0b0010001;
    font_monogram_data[172][ 9] = 0b0001110;
    font_monogram_chars[0xf0] = 172;
    // 'ñ'
    font_monogram_data[173][ 2] = 0b0010110;
    font_monogram_data[173][ 3] = 0b0001001;
    font_monogram_data[173][ 5] = 0b0001111;
    font_monogram_data[173][ 6] = 0b0010001;
    font_monogram_data[173][ 7] = 0b0010001;
    font_monogram_data[173][ 8] = 0b0010001;
    font_monogram_data[173][ 9] = 0b0010001;
    font_monogram_chars[0xf1] = 173;
    // 'ò'
    font_monogram_data[174][ 2] = 0b0000010;
    font_monogram_data[174][ 3] = 0b0000100;
    font_monogram_data[174][ 5] = 0b0001110;
    font_monogram_data[174][ 6] = 0b0010001;
    font_monogram_data[174][ 7] = 0b0010001;
    font_monogram_data[174][ 8] = 0b0010001;
    font_monogram_data[174][ 9] = 0b0001110;
    font_monogram_chars[0xf2] = 174;
    // 'ó'
    font_monogram_data[175][ 2] = 0b0001000;
    font_monogram_data[175][ 3] = 0b0000100;
    font_monogram_data[175][ 5] = 0b0001110;
    font_monogram_data[175][ 6] = 0b0010001;
    font_monogram_data[175][ 7] = 0b0010001;
    font_monogram_data[175][ 8] = 0b0010001;
    font_monogram_data[175][ 9] = 0b0001110;
    font_monogram_chars[0xf3] = 175;
    // 'ô'
    font_monogram_data[176][ 2] = 0b0000100;
    font_monogram_data[176][ 3] = 0b0001010;
    font_monogram_data[176][ 5] = 0b0001110;
    font_monogram_data[176][ 6] = 0b0010001;
    font_monogram_data[176][ 7] = 0b0010001;
    font_monogram_data[176][ 8] = 0b0010001;
    font_monogram_data[176][ 9] = 0b0001110;
    font_monogram_chars[0xf4] = 176;
    // 'õ'
    font_monogram_data[177][ 2] = 0b0010110;
    font_monogram_data[177][ 3] = 0b0001001;
    font_monogram_data[177][ 5] = 0b0001110;
    font_monogram_data[177][ 6] = 0b0010001;
    font_monogram_data[177][ 7] = 0b0010001;
    font_monogram_data[177][ 8] = 0b0010001;
    font_monogram_data[177][ 9] = 0b0001110;
    font_monogram_chars[0xf5] = 177;
    // 'ö'
    font_monogram_data[178][ 3] = 0b0001010;
    font_monogram_data[178][ 5] = 0b0001110;
    font_monogram_data[178][ 6] = 0b0010001;
    font_monogram_data[178][ 7] = 0b0010001;
    font_monogram_data[178][ 8] = 0b0010001;
    font_monogram_data[178][ 9] = 0b0001110;
    font_monogram_chars[0xf6] = 178;
    // '÷'
    font_monogram_data[179][ 5] = 0b0000100;
    font_monogram_data[179][ 7] = 0b0011111;
    font_monogram_data[179][ 9] = 0b0000100;
    font_monogram_chars[0xf7] = 179;
    // 'ø'
    font_monogram_data[180][ 5] = 0b0010110;
    font_monogram_data[180][ 6] = 0b0001001;
    font_monogram_data[180][ 7] = 0b0010101;
    font_monogram_data[180][ 8] = 0b0010010;
    font_monogram_data[180][ 9] = 0b0001101;
    font_monogram_chars[0xf8] = 180;
    // 'ù'
    font_monogram_data[181][ 2] = 0b0000010;
    font_monogram_data[181][ 3] = 0b0000100;
    font_monogram_data[181][ 5] = 0b0010001;
    font_monogram_data[181][ 6] = 0b0010001;
    font_monogram_data[181][ 7] = 0b0010001;
    font_monogram_data[181][ 8] = 0b0010001;
    font_monogram_data[181][ 9] = 0b0011110;
    font_monogram_chars[0xf9] = 181;
    // 'ú'
    font_monogram_data[182][ 2] = 0b0001000;
    font_monogram_data[182][ 3] = 0b0000100;
    font_monogram_data[182][ 5] = 0b0010001;
    font_monogram_data[182][ 6] = 0b0010001;
    font_monogram_data[182][ 7] = 0b0010001;
    font_monogram_data[182][ 8] = 0b0010001;
    font_monogram_data[182][ 9] = 0b0011110;
    font_monogram_chars[0xfa] = 182;
    // 'û'
    font_monogram_data[183][ 2] = 0b0000100;
    font_monogram_data[183][ 3] = 0b0001010;
    font_monogram_data[183][ 5] = 0b0010001;
    font_monogram_data[183][ 6] = 0b0010001;
    font_monogram_data[183][ 7] = 0b0010001;
    font_monogram_data[183][ 8] = 0b0010001;
    font_monogram_data[183][ 9] = 0b0011110;
    font_monogram_chars[0xfb] = 183;
    // 'ü'
    font_monogram_data[184][ 3] = 0b0001010;
    font_monogram_data[184][ 5] = 0b0010001;
    font_monogram_data[184][ 6] = 0b0010001;
    font_monogram_data[184][ 7] = 0b0010001;
    font_monogram_data[184][ 8] = 0b0010001;
    font_monogram_data[184][ 9] = 0b0011110;
    font_monogram_chars[0xfc] = 184;
    // 'ý'
    font_monogram_data[185][ 2] = 0b0001000;
    font_monogram_data[185][ 3] = 0b0000100;
    font_monogram_data[185][ 5] = 0b0010001;
    font_monogram_data[185][ 6] = 0b0010001;
    font_monogram_data[185][ 7] = 0b0010001;
    font_monogram_data[185][ 8] = 0b0010001;
    font_monogram_data[185][ 9] = 0b0011110;
    font_monogram_data[185][10] = 0b0010000;
    font_monogram_data[185][11] = 0b0001110;
    font_monogram_chars[0xfd] = 185;
    // 'þ'
    font_monogram_data[186][ 3] = 0b0000001;
    font_monogram_data[186][ 4] = 0b0000001;
    font_monogram_data[186][ 5] = 0b0001111;
    font_monogram_data[186][ 6] = 0b0010001;
    font_monogram_data[186][ 7] = 0b0010001;
    font_monogram_data[186][ 8] = 0b0010001;
    font_monogram_data[186][ 9] = 0b0001111;
    font_monogram_data[186][10] = 0b0000001;
    font_monogram_data[186][11] = 0b0000001;
    font_monogram_chars[0xfe] = 186;
    // 'ÿ'
    font_monogram_data[187][ 3] = 0b0001010;
    font_monogram_data[187][ 5] = 0b0010001;
    font_monogram_data[187][ 6] = 0b0010001;
    font_monogram_data[187][ 7] = 0b0010001;
    font_monogram_data[187][ 8] = 0b0010001;
    font_monogram_data[187][ 9] = 0b0011110;
    font_monogram_data[187][10] = 0b0010000;
    font_monogram_data[187][11] = 0b0001110;
    font_monogram_chars[0xff] = 187;
    // 'Ā'
    font_monogram_data[188][ 1] = 0b0001110;
    font_monogram_data[188][ 3] = 0b0001110;
    font_monogram_data[188][ 4] = 0b0010001;
    font_monogram_data[188][ 5] = 0b0010001;
    font_monogram_data[188][ 6] = 0b0011111;
    font_monogram_data[188][ 7] = 0b0010001;
    font_monogram_data[188][ 8] = 0b0010001;
    font_monogram_data[188][ 9] = 0b0010001;
    font_monogram_chars[0x100] = 188;
    // 'ā'
    font_monogram_data[189][ 3] = 0b0001110;
    font_monogram_data[189][ 5] = 0b0011110;
    font_monogram_data[189][ 6] = 0b0010001;
    font_monogram_data[189][ 7] = 0b0010001;
    font_monogram_data[189][ 8] = 0b0010001;
    font_monogram_data[189][ 9] = 0b0011110;
    font_monogram_chars[0x101] = 189;
    // 'Ă'
    font_monogram_data[190][ 0] = 0b0001010;
    font_monogram_data[190][ 1] = 0b0000100;
    font_monogram_data[190][ 3] = 0b0001110;
    font_monogram_data[190][ 4] = 0b0010001;
    font_monogram_data[190][ 5] = 0b0010001;
    font_monogram_data[190][ 6] = 0b0011111;
    font_monogram_data[190][ 7] = 0b0010001;
    font_monogram_data[190][ 8] = 0b0010001;
    font_monogram_data[190][ 9] = 0b0010001;
    font_monogram_chars[0x102] = 190;
    // 'ă'
    font_monogram_data[191][ 2] = 0b0001010;
    font_monogram_data[191][ 3] = 0b0000100;
    font_monogram_data[191][ 5] = 0b0011110;
    font_monogram_data[191][ 6] = 0b0010001;
    font_monogram_data[191][ 7] = 0b0010001;
    font_monogram_data[191][ 8] = 0b0010001;
    font_monogram_data[191][ 9] = 0b0011110;
    font_monogram_chars[0x103] = 191;
    // 'Ą'
    font_monogram_data[192][ 3] = 0b0001110;
    font_monogram_data[192][ 4] = 0b0010001;
    font_monogram_data[192][ 5] = 0b0010001;
    font_monogram_data[192][ 6] = 0b0011111;
    font_monogram_data[192][ 7] = 0b0010001;
    font_monogram_data[192][ 8] = 0b0010001;
    font_monogram_data[192][ 9] = 0b0010001;
    font_monogram_data[192][10] = 0b0001000;
    font_monogram_data[192][11] = 0b0010000;
    font_monogram_chars[0x104] = 192;
    // 'ą'
    font_monogram_data[193][ 5] = 0b0011110;
    font_monogram_data[193][ 6] = 0b0010001;
    font_monogram_data[193][ 7] = 0b0010001;
    font_monogram_data[193][ 8] = 0b0010001;
    font_monogram_data[193][ 9] = 0b0011110;
    font_monogram_data[193][10] = 0b0000100;
    font_monogram_data[193][11] = 0b0011000;
    font_monogram_chars[0x105] = 193;
    // 'Ć'
    font_monogram_data[194][ 0] = 0b0001000;
    font_monogram_data[194][ 1] = 0b0000100;
    font_monogram_data[194][ 3] = 0b0001110;
    font_monogram_data[194][ 4] = 0b0010001;
    font_monogram_data[194][ 5] = 0b0000001;
    font_monogram_data[194][ 6] = 0b0000001;
    font_monogram_data[194][ 7] = 0b0000001;
    font_monogram_data[194][ 8] = 0b0010001;
    font_monogram_data[194][ 9] = 0b0001110;
    font_monogram_chars[0x106] = 194;
    // 'ć'
    font_monogram_data[195][ 2] = 0b0001000;
    font_monogram_data[195][ 3] = 0b0000100;
    font_monogram_data[195][ 5] = 0b0001110;
    font_monogram_data[195][ 6] = 0b0010001;
    font_monogram_data[195][ 7] = 0b0000001;
    font_monogram_data[195][ 8] = 0b0010001;
    font_monogram_data[195][ 9] = 0b0001110;
    font_monogram_chars[0x107] = 195;
    // 'Ĉ'
    font_monogram_data[196][ 0] = 0b0000100;
    font_monogram_data[196][ 1] = 0b0001010;
    font_monogram_data[196][ 3] = 0b0001110;
    font_monogram_data[196][ 4] = 0b0010001;
    font_monogram_data[196][ 5] = 0b0000001;
    font_monogram_data[196][ 6] = 0b0000001;
    font_monogram_data[196][ 7] = 0b0000001;
    font_monogram_data[196][ 8] = 0b0010001;
    font_monogram_data[196][ 9] = 0b0001110;
    font_monogram_chars[0x108] = 196;
    // 'ĉ'
    font_monogram_data[197][ 2] = 0b0000100;
    font_monogram_data[197][ 3] = 0b0001010;
    font_monogram_data[197][ 5] = 0b0001110;
    font_monogram_data[197][ 6] = 0b0010001;
    font_monogram_data[197][ 7] = 0b0000001;
    font_monogram_data[197][ 8] = 0b0010001;
    font_monogram_data[197][ 9] = 0b0001110;
    font_monogram_chars[0x109] = 197;
    // 'Ċ'
    font_monogram_data[198][ 1] = 0b0000100;
    font_monogram_data[198][ 3] = 0b0001110;
    font_monogram_data[198][ 4] = 0b0010001;
    font_monogram_data[198][ 5] = 0b0000001;
    font_monogram_data[198][ 6] = 0b0000001;
    font_monogram_data[198][ 7] = 0b0000001;
    font_monogram_data[198][ 8] = 0b0010001;
    font_monogram_data[198][ 9] = 0b0001110;
    font_monogram_chars[0x10a] = 198;
    // 'ċ'
    font_monogram_data[199][ 3] = 0b0000100;
    font_monogram_data[199][ 5] = 0b0001110;
    font_monogram_data[199][ 6] = 0b0010001;
    font_monogram_data[199][ 7] = 0b0000001;
    font_monogram_data[199][ 8] = 0b0010001;
    font_monogram_data[199][ 9] = 0b0001110;
    font_monogram_chars[0x10b] = 199;
    // 'Č'
    font_monogram_data[200][ 0] = 0b0001010;
    font_monogram_data[200][ 1] = 0b0000100;
    font_monogram_data[200][ 3] = 0b0001110;
    font_monogram_data[200][ 4] = 0b0010001;
    font_monogram_data[200][ 5] = 0b0000001;
    font_monogram_data[200][ 6] = 0b0000001;
    font_monogram_data[200][ 7] = 0b0000001;
    font_monogram_data[200][ 8] = 0b0010001;
    font_monogram_data[200][ 9] = 0b0001110;
    font_monogram_chars[0x10c] = 200;
    // 'č'
    font_monogram_data[201][ 2] = 0b0001010;
    font_monogram_data[201][ 3] = 0b0000100;
    font_monogram_data[201][ 5] = 0b0001110;
    font_monogram_data[201][ 6] = 0b0010001;
    font_monogram_data[201][ 7] = 0b0000001;
    font_monogram_data[201][ 8] = 0b0010001;
    font_monogram_data[201][ 9] = 0b0001110;
    font_monogram_chars[0x10d] = 201;
    // 'Ď'
    font_monogram_data[202][ 0] = 0b0001010;
    font_monogram_data[202][ 1] = 0b0000100;
    font_monogram_data[202][ 3] = 0b0001111;
    font_monogram_data[202][ 4] = 0b0010001;
    font_monogram_data[202][ 5] = 0b0010001;
    font_monogram_data[202][ 6] = 0b0010001;
    font_monogram_data[202][ 7] = 0b0010001;
    font_monogram_data[202][ 8] = 0b0010001;
    font_monogram_data[202][ 9] = 0b0001111;
    font_monogram_chars[0x10e] = 202;
    // 'ď'
    font_monogram_data[203][ 2] = 0b1010000;
    font_monogram_data[203][ 3] = 0b1010000;
    font_monogram_data[203][ 4] = 0b0010000;
    font_monogram_data[203][ 5] = 0b0011110;
    font_monogram_data[203][ 6] = 0b0010001;
    font_monogram_data[203][ 7] = 0b0010001;
    font_monogram_data[203][ 8] = 0b0010001;
    font_monogram_data[203][ 9] = 0b0011110;
    font_monogram_chars[0x10f] = 203;
    // 'Đ'
    font_monogram_data[204][ 3] = 0b0001111;
    font_monogram_data[204][ 4] = 0b0010001;
    font_monogram_data[204][ 5] = 0b0010001;
    font_monogram_data[204][ 6] = 0b0010011;
    font_monogram_data[204][ 7] = 0b0010001;
    font_monogram_data[204][ 8] = 0b0010001;
    font_monogram_data[204][ 9] = 0b0001111;
    font_monogram_chars[0x110] = 204;
    // 'đ'
    font_monogram_data[205][ 2] = 0b0010000;
    font_monogram_data[205][ 3] = 0b0111100;
    font_monogram_data[205][ 4] = 0b0010000;
    font_monogram_data[205][ 5] = 0b0011110;
    font_monogram_data[205][ 6] = 0b0010001;
    font_monogram_data[205][ 7] = 0b0010001;
    font_monogram_data[205][ 8] = 0b0010001;
    font_monogram_data[205][ 9] = 0b0011110;
    font_monogram_chars[0x111] = 205;
    // 'Ē'
    font_monogram_data[206][ 1] = 0b0001110;
    font_monogram_data[206][ 3] = 0b0011111;
    font_monogram_data[206][ 4] = 0b0000001;
    font_monogram_data[206][ 5] = 0b0000001;
    font_monogram_data[206][ 6] = 0b0000111;
    font_monogram_data[206][ 7] = 0b0000001;
    font_monogram_data[206][ 8] = 0b0000001;
    font_monogram_data[206][ 9] = 0b0011111;
    font_monogram_chars[0x112] = 206;
    // 'ē'
    font_monogram_data[207][ 3] = 0b0001110;
    font_monogram_data[207][ 5] = 0b0001110;
    font_monogram_data[207][ 6] = 0b0010001;
    font_monogram_data[207][ 7] = 0b0011111;
    font_monogram_data[207][ 8] = 0b0000001;
    font_monogram_data[207][ 9] = 0b0001110;
    font_monogram_chars[0x113] = 207;
    // 'Ĕ'
    font_monogram_data[208][ 0] = 0b0001010;
    font_monogram_data[208][ 1] = 0b0000100;
    font_monogram_data[208][ 3] = 0b0011111;
    font_monogram_data[208][ 4] = 0b0000001;
    font_monogram_data[208][ 5] = 0b0000001;
    font_monogram_data[208][ 6] = 0b0000111;
    font_monogram_data[208][ 7] = 0b0000001;
    font_monogram_data[208][ 8] = 0b0000001;
    font_monogram_data[208][ 9] = 0b0011111;
    font_monogram_chars[0x114] = 208;
    // 'ĕ'
    font_monogram_data[209][ 2] = 0b0001010;
    font_monogram_data[209][ 3] = 0b0000100;
    font_monogram_data[209][ 5] = 0b0001110;
    font_monogram_data[209][ 6] = 0b0010001;
    font_monogram_data[209][ 7] = 0b0011111;
    font_monogram_data[209][ 8] = 0b0000001;
    font_monogram_data[209][ 9] = 0b0001110;
    font_monogram_chars[0x115] = 209;
    // 'Ė'
    font_monogram_data[210][ 1] = 0b0000100;
    font_monogram_data[210][ 3] = 0b0011111;
    font_monogram_data[210][ 4] = 0b0000001;
    font_monogram_data[210][ 5] = 0b0000001;
    font_monogram_data[210][ 6] = 0b0000111;
    font_monogram_data[210][ 7] = 0b0000001;
    font_monogram_data[210][ 8] = 0b0000001;
    font_monogram_data[210][ 9] = 0b0011111;
    font_monogram_chars[0x116] = 210;
    // 'ė'
    font_monogram_data[211][ 3] = 0b0000100;
    font_monogram_data[211][ 5] = 0b0001110;
    font_monogram_data[211][ 6] = 0b0010001;
    font_monogram_data[211][ 7] = 0b0011111;
    font_monogram_data[211][ 8] = 0b0000001;
    font_monogram_data[211][ 9] = 0b0001110;
    font_monogram_chars[0x117] = 211;
    // 'Ę'
    font_monogram_data[212][ 3] = 0b0011111;
    font_monogram_data[212][ 4] = 0b0000001;
    font_monogram_data[212][ 5] = 0b0000001;
    font_monogram_data[212][ 6] = 0b0000111;
    font_monogram_data[212][ 7] = 0b0000001;
    font_monogram_data[212][ 8] = 0b0000001;
    font_monogram_data[212][ 9] = 0b0011111;
    font_monogram_data[212][10] = 0b0000100;
    font_monogram_data[212][11] = 0b0011000;
    font_monogram_chars[0x118] = 212;
    // 'ę'
    font_monogram_data[213][ 5] = 0b0001110;
    font_monogram_data[213][ 6] = 0b0010001;
    font_monogram_data[213][ 7] = 0b0011111;
    font_monogram_data[213][ 8] = 0b0000001;
    font_monogram_data[213][ 9] = 0b0011110;
    font_monogram_data[213][10] = 0b0000100;
    font_monogram_data[213][11] = 0b0011000;
    font_monogram_chars[0x119] = 213;
    // 'Ě'
    font_monogram_data[214][ 1] = 0b0001110;
    font_monogram_data[214][ 3] = 0b0011111;
    font_monogram_data[214][ 4] = 0b0000001;
    font_monogram_data[214][ 5] = 0b0000001;
    font_monogram_data[214][ 6] = 0b0000111;
    font_monogram_data[214][ 7] = 0b0000001;
    font_monogram_data[214][ 8] = 0b0000001;
    font_monogram_data[214][ 9] = 0b0011111;
    font_monogram_chars[0x11a] = 214;
    // 'ě'
    font_monogram_data[215][ 3] = 0b0001010;
    font_monogram_data[215][ 5] = 0b0001110;
    font_monogram_data[215][ 6] = 0b0010001;
    font_monogram_data[215][ 7] = 0b0011111;
    font_monogram_data[215][ 8] = 0b0000001;
    font_monogram_data[215][ 9] = 0b0001110;
    font_monogram_chars[0x11b] = 215;
    // 'Ĝ'
    font_monogram_data[216][ 0] = 0b0000100;
    font_monogram_data[216][ 1] = 0b0001010;
    font_monogram_data[216][ 3] = 0b0001110;
    font_monogram_data[216][ 4] = 0b0010001;
    font_monogram_data[216][ 5] = 0b0000001;
    font_monogram_data[216][ 6] = 0b0011101;
    font_monogram_data[216][ 7] = 0b0010001;
    font_monogram_data[216][ 8] = 0b0010001;
    font_monogram_data[216][ 9] = 0b0001110;
    font_monogram_chars[0x11c] = 216;
    // 'ĝ'
    font_monogram_data[217][ 2] = 0b0000100;
    font_monogram_data[217][ 3] = 0b0001010;
    font_monogram_data[217][ 5] = 0b0011110;
    font_monogram_data[217][ 6] = 0b0010001;
    font_monogram_data[217][ 7] = 0b0010001;
    font_monogram_data[217][ 8] = 0b0010001;
    font_monogram_data[217][ 9] = 0b0011110;
    font_monogram_data[217][10] = 0b0010000;
    font_monogram_data[217][11] = 0b0001110;
    font_monogram_chars[0x11d] = 217;
    // 'Ğ'
    font_monogram_data[218][ 0] = 0b0001010;
    font_monogram_data[218][ 1] = 0b0000100;
    font_monogram_data[218][ 3] = 0b0001110;
    font_monogram_data[218][ 4] = 0b0010001;
    font_monogram_data[218][ 5] = 0b0000001;
    font_monogram_data[218][ 6] = 0b0011101;
    font_monogram_data[218][ 7] = 0b0010001;
    font_monogram_data[218][ 8] = 0b0010001;
    font_monogram_data[218][ 9] = 0b0001110;
    font_monogram_chars[0x11e] = 218;
    // 'ğ'
    font_monogram_data[219][ 2] = 0b0001010;
    font_monogram_data[219][ 3] = 0b0000100;
    font_monogram_data[219][ 5] = 0b0011110;
    font_monogram_data[219][ 6] = 0b0010001;
    font_monogram_data[219][ 7] = 0b0010001;
    font_monogram_data[219][ 8] = 0b0010001;
    font_monogram_data[219][ 9] = 0b0011110;
    font_monogram_data[219][10] = 0b0010000;
    font_monogram_data[219][11] = 0b0001110;
    font_monogram_chars[0x11f] = 219;
    // 'Ġ'
    font_monogram_data[220][ 1] = 0b0000100;
    font_monogram_data[220][ 3] = 0b0001110;
    font_monogram_data[220][ 4] = 0b0010001;
    font_monogram_data[220][ 5] = 0b0000001;
    font_monogram_data[220][ 6] = 0b0011101;
    font_monogram_data[220][ 7] = 0b0010001;
    font_monogram_data[220][ 8] = 0b0010001;
    font_monogram_data[220][ 9] = 0b0001110;
    font_monogram_chars[0x120] = 220;
    // 'ġ'
    font_monogram_data[221][ 3] = 0b0000100;
    font_monogram_data[221][ 5] = 0b0011110;
    font_monogram_data[221][ 6] = 0b0010001;
    font_monogram_data[221][ 7] = 0b0010001;
    font_monogram_data[221][ 8] = 0b0010001;
    font_monogram_data[221][ 9] = 0b0011110;
    font_monogram_data[221][10] = 0b0010000;
    font_monogram_data[221][11] = 0b0001110;
    font_monogram_chars[0x121] = 221;
    // 'Ģ'
    font_monogram_data[222][ 3] = 0b0001110;
    font_monogram_data[222][ 4] = 0b0010001;
    font_monogram_data[222][ 5] = 0b0000001;
    font_monogram_data[222][ 6] = 0b0011101;
    font_monogram_data[222][ 7] = 0b0010001;
    font_monogram_data[222][ 8] = 0b0010001;
    font_monogram_data[222][ 9] = 0b0001110;
    font_monogram_data[222][10] = 0b0001000;
    font_monogram_data[222][11] = 0b0000110;
    font_monogram_chars[0x122] = 222;
    // 'ģ'
    font_monogram_data[223][ 2] = 0b0001000;
    font_monogram_data[223][ 3] = 0b0000100;
    font_monogram_data[223][ 5] = 0b0011110;
    font_monogram_data[223][ 6] = 0b0010001;
    font_monogram_data[223][ 7] = 0b0010001;
    font_monogram_data[223][ 8] = 0b0010001;
    font_monogram_data[223][ 9] = 0b0011110;
    font_monogram_data[223][10] = 0b0010000;
    font_monogram_data[223][11] = 0b0001110;
    font_monogram_chars[0x123] = 223;
    // 'Ĥ'
    font_monogram_data[224][ 0] = 0b0000100;
    font_monogram_data[224][ 1] = 0b0001010;
    font_monogram_data[224][ 3] = 0b0010001;
    font_monogram_data[224][ 4] = 0b0010001;
    font_monogram_data[224][ 5] = 0b0010001;
    font_monogram_data[224][ 6] = 0b0011111;
    font_monogram_data[224][ 7] = 0b0010001;
    font_monogram_data[224][ 8] = 0b0010001;
    font_monogram_data[224][ 9] = 0b0010001;
    font_monogram_chars[0x124] = 224;
    // 'ĥ'
    font_monogram_data[225][ 2] = 0b0001000;
    font_monogram_data[225][ 3] = 0b0010101;
    font_monogram_data[225][ 4] = 0b0000001;
    font_monogram_data[225][ 5] = 0b0001111;
    font_monogram_data[225][ 6] = 0b0010001;
    font_monogram_data[225][ 7] = 0b0010001;
    font_monogram_data[225][ 8] = 0b0010001;
    font_monogram_data[225][ 9] = 0b0010001;
    font_monogram_chars[0x125] = 225;
    // 'Ħ'
    font_monogram_data[226][ 3] = 0b0010001;
    font_monogram_data[226][ 4] = 0b0111111;
    font_monogram_data[226][ 5] = 0b0010001;
    font_monogram_data[226][ 6] = 0b0011111;
    font_monogram_data[226][ 7] = 0b0010001;
    font_monogram_data[226][ 8] = 0b0010001;
    font_monogram_data[226][ 9] = 0b0010001;
    font_monogram_chars[0x126] = 226;
    // 'ħ'
    font_monogram_data[227][ 3] = 0b0000001;
    font_monogram_data[227][ 4] = 0b0000011;
    font_monogram_data[227][ 5] = 0b0000001;
    font_monogram_data[227][ 6] = 0b0001111;
    font_monogram_data[227][ 7] = 0b0010001;
    font_monogram_data[227][ 8] = 0b0010001;
    font_monogram_data[227][ 9] = 0b0010001;
    font_monogram_chars[0x127] = 227;
    // 'Ĩ'
    font_monogram_data[228][ 0] = 0b0010110;
    font_monogram_data[228][ 1] = 0b0001001;
    font_monogram_data[228][ 3] = 0b0011111;
    font_monogram_data[228][ 4] = 0b0000100;
    font_monogram_data[228][ 5] = 0b0000100;
    font_monogram_data[228][ 6] = 0b0000100;
    font_monogram_data[228][ 7] = 0b0000100;
    font_monogram_data[228][ 8] = 0b0000100;
    font_monogram_data[228][ 9] = 0b0011111;
    font_monogram_chars[0x128] = 228;
    // 'ĩ'
    font_monogram_data[229][ 2] = 0b0010110;
    font_monogram_data[229][ 3] = 0b0001001;
    font_monogram_data[229][ 5] = 0b0000110;
    font_monogram_data[229][ 6] = 0b0000100;
    font_monogram_data[229][ 7] = 0b0000100;
    font_monogram_data[229][ 8] = 0b0000100;
    font_monogram_data[229][ 9] = 0b0011111;
    font_monogram_chars[0x129] = 229;
    // 'Ī'
    font_monogram_data[230][ 1] = 0b0001110;
    font_monogram_data[230][ 3] = 0b0011111;
    font_monogram_data[230][ 4] = 0b0000100;
    font_monogram_data[230][ 5] = 0b0000100;
    font_monogram_data[230][ 6] = 0b0000100;
    font_monogram_data[230][ 7] = 0b0000100;
    font_monogram_data[230][ 8] = 0b0000100;
    font_monogram_data[230][ 9] = 0b0011111;
    font_monogram_chars[0x12a] = 230;
    // 'ī'
    font_monogram_data[231][ 3] = 0b0001110;
    font_monogram_data[231][ 5] = 0b0000110;
    font_monogram_data[231][ 6] = 0b0000100;
    font_monogram_data[231][ 7] = 0b0000100;
    font_monogram_data[231][ 8] = 0b0000100;
    font_monogram_data[231][ 9] = 0b0011111;
    font_monogram_chars[0x12b] = 231;
    // 'Ĭ'
    font_monogram_data[232][ 0] = 0b0001010;
    font_monogram_data[232][ 1] = 0b0000100;
    font_monogram_data[232][ 3] = 0b0011111;
    font_monogram_data[232][ 4] = 0b0000100;
    font_monogram_data[232][ 5] = 0b0000100;
    font_monogram_data[232][ 6] = 0b0000100;
    font_monogram_data[232][ 7] = 0b0000100;
    font_monogram_data[232][ 8] = 0b0000100;
    font_monogram_data[232][ 9] = 0b0011111;
    font_monogram_chars[0x12c] = 232;
    // 'ĭ'
    font_monogram_data[233][ 2] = 0b0001010;
    font_monogram_data[233][ 3] = 0b0000100;
    font_monogram_data[233][ 5] = 0b0000110;
    font_monogram_data[233][ 6] = 0b0000100;
    font_monogram_data[233][ 7] = 0b0000100;
    font_monogram_data[233][ 8] = 0b0000100;
    font_monogram_data[233][ 9] = 0b0011111;
    font_monogram_chars[0x12d] = 233;
    // 'Į'
    font_monogram_data[234][ 3] = 0b0011111;
    font_monogram_data[234][ 4] = 0b0000100;
    font_monogram_data[234][ 5] = 0b0000100;
    font_monogram_data[234][ 6] = 0b0000100;
    font_monogram_data[234][ 7] = 0b0000100;
    font_monogram_data[234][ 8] = 0b0000100;
    font_monogram_data[234][ 9] = 0b0011111;
    font_monogram_data[234][10] = 0b0000100;
    font_monogram_data[234][11] = 0b0011000;
    font_monogram_chars[0x12e] = 234;
    // 'į'
    font_monogram_data[235][ 3] = 0b0000100;
    font_monogram_data[235][ 5] = 0b0000110;
    font_monogram_data[235][ 6] = 0b0000100;
    font_monogram_data[235][ 7] = 0b0000100;
    font_monogram_data[235][ 8] = 0b0000100;
    font_monogram_data[235][ 9] = 0b0011111;
    font_monogram_data[235][10] = 0b0000100;
    font_monogram_data[235][11] = 0b0011000;
    font_monogram_chars[0x12f] = 235;
    // 'İ'
    font_monogram_data[236][ 0] = 0b0010110;
    font_monogram_data[236][ 1] = 0b0001001;
    font_monogram_data[236][ 3] = 0b0011111;
    font_monogram_data[236][ 4] = 0b0000100;
    font_monogram_data[236][ 5] = 0b0000100;
    font_monogram_data[236][ 6] = 0b0000100;
    font_monogram_data[236][ 7] = 0b0000100;
    font_monogram_data[236][ 8] = 0b0000100;
    font_monogram_data[236][ 9] = 0b0011111;
    font_monogram_chars[0x130] = 236;
    // 'ı'
    font_monogram_data[237][ 5] = 0b0000110;
    font_monogram_data[237][ 6] = 0b0000100;
    font_monogram_data[237][ 7] = 0b0000100;
    font_monogram_data[237][ 8] = 0b0000100;
    font_monogram_data[237][ 9] = 0b0011111;
    font_monogram_chars[0x131] = 237;
    // 'Ĳ'
    font_monogram_data[238][ 3] = 0b0010111;
    font_monogram_data[238][ 4] = 0b0010010;
    font_monogram_data[238][ 5] = 0b0010010;
    font_monogram_data[238][ 6] = 0b0010010;
    font_monogram_data[238][ 7] = 0b0010010;
    font_monogram_data[238][ 8] = 0b0010010;
    font_monogram_data[238][ 9] = 0b0001111;
    font_monogram_chars[0x132] = 238;
    // 'ĳ'
    font_monogram_data[239][ 3] = 0b0010010;
    font_monogram_data[239][ 5] = 0b0011011;
    font_monogram_data[239][ 6] = 0b0010010;
    font_monogram_data[239][ 7] = 0b0010010;
    font_monogram_data[239][ 8] = 0b0010010;
    font_monogram_data[239][ 9] = 0b0011111;
    font_monogram_data[239][10] = 0b0010000;
    font_monogram_data[239][11] = 0b0001110;
    font_monogram_chars[0x133] = 239;
    // 'Ĵ'
    font_monogram_data[240][ 0] = 0b0000100;
    font_monogram_data[240][ 1] = 0b0001010;
    font_monogram_data[240][ 3] = 0b0010000;
    font_monogram_data[240][ 4] = 0b0010000;
    font_monogram_data[240][ 5] = 0b0010000;
    font_monogram_data[240][ 6] = 0b0010000;
    font_monogram_data[240][ 7] = 0b0010001;
    font_monogram_data[240][ 8] = 0b0010001;
    font_monogram_data[240][ 9] = 0b0001110;
    font_monogram_chars[0x134] = 240;
    // 'ĵ'
    font_monogram_data[241][ 2] = 0b0010000;
    font_monogram_data[241][ 3] = 0b0101000;
    font_monogram_data[241][ 5] = 0b0011000;
    font_monogram_data[241][ 6] = 0b0010000;
    font_monogram_data[241][ 7] = 0b0010000;
    font_monogram_data[241][ 8] = 0b0010000;
    font_monogram_data[241][ 9] = 0b0010000;
    font_monogram_data[241][10] = 0b0010001;
    font_monogram_data[241][11] = 0b0001110;
    font_monogram_chars[0x135] = 241;
    // 'Ķ'
    font_monogram_data[242][ 3] = 0b0010001;
    font_monogram_data[242][ 4] = 0b0001001;
    font_monogram_data[242][ 5] = 0b0000101;
    font_monogram_data[242][ 6] = 0b0000011;
    font_monogram_data[242][ 7] = 0b0000101;
    font_monogram_data[242][ 8] = 0b0001001;
    font_monogram_data[242][ 9] = 0b0010001;
    font_monogram_data[242][10] = 0b0000100;
    font_monogram_data[242][11] = 0b0000100;
    font_monogram_chars[0x136] = 242;
    // 'ķ'
    font_monogram_data[243][ 3] = 0b0000001;
    font_monogram_data[243][ 4] = 0b0000001;
    font_monogram_data[243][ 5] = 0b0010001;
    font_monogram_data[243][ 6] = 0b0001001;
    font_monogram_data[243][ 7] = 0b0000111;
    font_monogram_data[243][ 8] = 0b0001001;
    font_monogram_data[243][ 9] = 0b0010001;
    font_monogram_data[243][10] = 0b0000100;
    font_monogram_data[243][11] = 0b0000100;
    font_monogram_chars[0x137] = 243;
    // 'ĸ'
    font_monogram_data[244][ 5] = 0b0010001;
    font_monogram_data[244][ 6] = 0b0001001;
    font_monogram_data[244][ 7] = 0b0000111;
    font_monogram_data[244][ 8] = 0b0001001;
    font_monogram_data[244][ 9] = 0b0010001;
    font_monogram_chars[0x138] = 244;
    // 'Ĺ'
    font_monogram_data[245][ 0] = 0b0001000;
    font_monogram_data[245][ 1] = 0b0000100;
    font_monogram_data[245][ 3] = 0b0000001;
    font_monogram_data[245][ 4] = 0b0000001;
    font_monogram_data[245][ 5] = 0b0000001;
    font_monogram_data[245][ 6] = 0b0000001;
    font_monogram_data[245][ 7] = 0b0000001;
    font_monogram_data[245][ 8] = 0b0000001;
    font_monogram_data[245][ 9] = 0b0011111;
    font_monogram_chars[0x139] = 245;
    // 'ĺ'
    font_monogram_data[246][ 0] = 0b0001000;
    font_monogram_data[246][ 1] = 0b0000100;
    font_monogram_data[246][ 3] = 0b0011111;
    font_monogram_data[246][ 4] = 0b0000100;
    font_monogram_data[246][ 5] = 0b0000100;
    font_monogram_data[246][ 6] = 0b0000100;
    font_monogram_data[246][ 7] = 0b0000100;
    font_monogram_data[246][ 8] = 0b0000100;
    font_monogram_data[246][ 9] = 0b0011111;
    font_monogram_chars[0x13a] = 246;
    // 'Ļ'
    font_monogram_data[247][ 3] = 0b0000001;
    font_monogram_data[247][ 4] = 0b0000001;
    font_monogram_data[247][ 5] = 0b0000001;
    font_monogram_data[247][ 6] = 0b0000001;
    font_monogram_data[247][ 7] = 0b0000001;
    font_monogram_data[247][ 8] = 0b0000001;
    font_monogram_data[247][ 9] = 0b0011111;
    font_monogram_data[247][10] = 0b0001000;
    font_monogram_data[247][11] = 0b0000110;
    font_monogram_chars[0x13b] = 247;
    // 'ļ'
    font_monogram_data[248][ 3] = 0b0011111;
    font_monogram_data[248][ 4] = 0b0000100;
    font_monogram_data[248][ 5] = 0b0000100;
    font_monogram_data[248][ 6] = 0b0000100;
    font_monogram_data[248][ 7] = 0b0000100;
    font_monogram_data[248][ 8] = 0b0000100;
    font_monogram_data[248][ 9] = 0b0011111;
    font_monogram_data[248][10] = 0b0001000;
    font_monogram_data[248][11] = 0b0000110;
    font_monogram_chars[0x13c] = 248;
    // 'Ľ'
    font_monogram_data[249][ 3] = 0b0010001;
    font_monogram_data[249][ 4] = 0b0010001;
    font_monogram_data[249][ 5] = 0b0001001;
    font_monogram_data[249][ 6] = 0b0000001;
    font_monogram_data[249][ 7] = 0b0000001;
    font_monogram_data[249][ 8] = 0b0000001;
    font_monogram_data[249][ 9] = 0b0011111;
    font_monogram_chars[0x13d] = 249;
    // 'ľ'
    font_monogram_data[250][ 3] = 0b0010011;
    font_monogram_data[250][ 4] = 0b0010010;
    font_monogram_data[250][ 5] = 0b0001010;
    font_monogram_data[250][ 6] = 0b0000010;
    font_monogram_data[250][ 7] = 0b0000010;
    font_monogram_data[250][ 8] = 0b0000010;
    font_monogram_data[250][ 9] = 0b0011100;
    font_monogram_chars[0x13e] = 250;
    // 'Ŀ'
    font_monogram_data[251][ 3] = 0b0000001;
    font_monogram_data[251][ 4] = 0b0000001;
    font_monogram_data[251][ 5] = 0b0000001;
    font_monogram_data[251][ 6] = 0b0001001;
    font_monogram_data[251][ 7] = 0b0000001;
    font_monogram_data[251][ 8] = 0b0000001;
    font_monogram_data[251][ 9] = 0b0011111;
    font_monogram_chars[0x13f] = 251;
    // 'ŀ'
    font_monogram_data[252][ 3] = 0b0000011;
    font_monogram_data[252][ 4] = 0b0000010;
    font_monogram_data[252][ 5] = 0b0000010;
    font_monogram_data[252][ 6] = 0b0001010;
    font_monogram_data[252][ 7] = 0b0000010;
    font_monogram_data[252][ 8] = 0b0000010;
    font_monogram_data[252][ 9] = 0b0011100;
    font_monogram_chars[0x140] = 252;
    // 'Ł'
    font_monogram_data[253][ 3] = 0b0000001;
    font_monogram_data[253][ 4] = 0b0000001;
    font_monogram_data[253][ 5] = 0b0000001;
    font_monogram_data[253][ 6] = 0b0000011;
    font_monogram_data[253][ 7] = 0b0000001;
    font_monogram_data[253][ 8] = 0b0000001;
    font_monogram_data[253][ 9] = 0b0011111;
    font_monogram_chars[0x141] = 253;
    // 'ł'
    font_monogram_data[254][ 3] = 0b0000011;
    font_monogram_data[254][ 4] = 0b0000010;
    font_monogram_data[254][ 5] = 0b0000010;
    font_monogram_data[254][ 6] = 0b0000110;
    font_monogram_data[254][ 7] = 0b0000011;
    font_monogram_data[254][ 8] = 0b0000010;
    font_monogram_data[254][ 9] = 0b0011100;
    font_monogram_chars[0x142] = 254;
    // 'Ń'
    font_monogram_data[255][ 0] = 0b0001000;
    font_monogram_data[255][ 1] = 0b0000100;
    font_monogram_data[255][ 3] = 0b0010001;
    font_monogram_data[255][ 4] = 0b0010001;
    font_monogram_data[255][ 5] = 0b0010011;
    font_monogram_data[255][ 6] = 0b0010101;
    font_monogram_data[255][ 7] = 0b0011001;
    font_monogram_data[255][ 8] = 0b0010001;
    font_monogram_data[255][ 9] = 0b0010001;
    font_monogram_chars[0x143] = 255;
    // 'ń'
    font_monogram_data[256][ 2] = 0b0001000;
    font_monogram_data[256][ 3] = 0b0000100;
    font_monogram_data[256][ 5] = 0b0001111;
    font_monogram_data[256][ 6] = 0b0010001;
    font_monogram_data[256][ 7] = 0b0010001;
    font_monogram_data[256][ 8] = 0b0010001;
    font_monogram_data[256][ 9] = 0b0010001;
    font_monogram_chars[0x144] = 256;
    // 'Ņ'
    font_monogram_data[257][ 3] = 0b0010001;
    font_monogram_data[257][ 4] = 0b0010001;
    font_monogram_data[257][ 5] = 0b0010011;
    font_monogram_data[257][ 6] = 0b0010101;
    font_monogram_data[257][ 7] = 0b0011001;
    font_monogram_data[257][ 8] = 0b0010001;
    font_monogram_data[257][ 9] = 0b0010001;
    font_monogram_data[257][10] = 0b0000100;
    font_monogram_data[257][11] = 0b0000011;
    font_monogram_chars[0x145] = 257;
    // 'ņ'
    font_monogram_data[258][ 5] = 0b0001111;
    font_monogram_data[258][ 6] = 0b0010001;
    font_monogram_data[258][ 7] = 0b0010001;
    font_monogram_data[258][ 8] = 0b0010001;
    font_monogram_data[258][ 9] = 0b0010001;
    font_monogram_data[258][10] = 0b0000100;
    font_monogram_data[258][11] = 0b0000011;
    font_monogram_chars[0x146] = 258;
    // 'Ň'
    font_monogram_data[259][ 0] = 0b0001010;
    font_monogram_data[259][ 1] = 0b0000100;
    font_monogram_data[259][ 3] = 0b0010001;
    font_monogram_data[259][ 4] = 0b0010001;
    font_monogram_data[259][ 5] = 0b0010011;
    font_monogram_data[259][ 6] = 0b0010101;
    font_monogram_data[259][ 7] = 0b0011001;
    font_monogram_data[259][ 8] = 0b0010001;
    font_monogram_data[259][ 9] = 0b0010001;
    font_monogram_chars[0x147] = 259;
    // 'ň'
    font_monogram_data[260][ 2] = 0b0001010;
    font_monogram_data[260][ 3] = 0b0000100;
    font_monogram_data[260][ 5] = 0b0001111;
    font_monogram_data[260][ 6] = 0b0010001;
    font_monogram_data[260][ 7] = 0b0010001;
    font_monogram_data[260][ 8] = 0b0010001;
    font_monogram_data[260][ 9] = 0b0010001;
    font_monogram_chars[0x148] = 260;
    // 'ŉ'
    font_monogram_data[261][ 5] = 0b0001111;
    font_monogram_data[261][ 6] = 0b0010001;
    font_monogram_data[261][ 7] = 0b0010001;
    font_monogram_data[261][ 8] = 0b0010001;
    font_monogram_data[261][ 9] = 0b0010001;
    font_monogram_chars[0x149] = 261;
    // 'Ŋ'
    font_monogram_data[262][ 3] = 0b0010001;
    font_monogram_data[262][ 4] = 0b0010001;
    font_monogram_data[262][ 5] = 0b0010011;
    font_monogram_data[262][ 6] = 0b0010101;
    font_monogram_data[262][ 7] = 0b0011001;
    font_monogram_data[262][ 8] = 0b0010001;
    font_monogram_data[262][ 9] = 0b0010001;
    font_monogram_data[262][10] = 0b0010000;
    font_monogram_data[262][11] = 0b0001100;
    font_monogram_chars[0x14a] = 262;
    // 'ŋ'
    font_monogram_data[263][ 5] = 0b0001111;
    font_monogram_data[263][ 6] = 0b0010001;
    font_monogram_data[263][ 7] = 0b0010001;
    font_monogram_data[263][ 8] = 0b0010001;
    font_monogram_data[263][ 9] = 0b0010001;
    font_monogram_data[263][10] = 0b0010000;
    font_monogram_data[263][11] = 0b0001100;
    font_monogram_chars[0x14b] = 263;
    // 'Ō'
    font_monogram_data[264][ 1] = 0b0001110;
    font_monogram_data[264][ 3] = 0b0001110;
    font_monogram_data[264][ 4] = 0b0010001;
    font_monogram_data[264][ 5] = 0b0010001;
    font_monogram_data[264][ 6] = 0b0010001;
    font_monogram_data[264][ 7] = 0b0010001;
    font_monogram_data[264][ 8] = 0b0010001;
    font_monogram_data[264][ 9] = 0b0001110;
    font_monogram_chars[0x14c] = 264;
    // 'ō'
    font_monogram_data[265][ 3] = 0b0001110;
    font_monogram_data[265][ 5] = 0b0001110;
    font_monogram_data[265][ 6] = 0b0010001;
    font_monogram_data[265][ 7] = 0b0010001;
    font_monogram_data[265][ 8] = 0b0010001;
    font_monogram_data[265][ 9] = 0b0001110;
    font_monogram_chars[0x14d] = 265;
    // 'Ŏ'
    font_monogram_data[266][ 0] = 0b0001010;
    font_monogram_data[266][ 1] = 0b0000100;
    font_monogram_data[266][ 3] = 0b0001110;
    font_monogram_data[266][ 4] = 0b0010001;
    font_monogram_data[266][ 5] = 0b0010001;
    font_monogram_data[266][ 6] = 0b0010001;
    font_monogram_data[266][ 7] = 0b0010001;
    font_monogram_data[266][ 8] = 0b0010001;
    font_monogram_data[266][ 9] = 0b0001110;
    font_monogram_chars[0x14e] = 266;
    // 'ŏ'
    font_monogram_data[267][ 2] = 0b0001010;
    font_monogram_data[267][ 3] = 0b0000100;
    font_monogram_data[267][ 5] = 0b0001110;
    font_monogram_data[267][ 6] = 0b0010001;
    font_monogram_data[267][ 7] = 0b0010001;
    font_monogram_data[267][ 8] = 0b0010001;
    font_monogram_data[267][ 9] = 0b0001110;
    font_monogram_chars[0x14f] = 267;
    // 'Ő'
    font_monogram_data[268][ 0] = 0b0010100;
    font_monogram_data[268][ 1] = 0b0001010;
    font_monogram_data[268][ 3] = 0b0001110;
    font_monogram_data[268][ 4] = 0b0010001;
    font_monogram_data[268][ 5] = 0b0010001;
    font_monogram_data[268][ 6] = 0b0010001;
    font_monogram_data[268][ 7] = 0b0010001;
    font_monogram_data[268][ 8] = 0b0010001;
    font_monogram_data[268][ 9] = 0b0001110;
    font_monogram_chars[0x150] = 268;
    // 'ő'
    font_monogram_data[269][ 2] = 0b0010100;
    font_monogram_data[269][ 3] = 0b0001010;
    font_monogram_data[269][ 5] = 0b0001110;
    font_monogram_data[269][ 6] = 0b0010001;
    font_monogram_data[269][ 7] = 0b0010001;
    font_monogram_data[269][ 8] = 0b0010001;
    font_monogram_data[269][ 9] = 0b0001110;
    font_monogram_chars[0x151] = 269;
    // 'Œ'
    font_monogram_data[270][ 3] = 0b0011110;
    font_monogram_data[270][ 4] = 0b0000101;
    font_monogram_data[270][ 5] = 0b0000101;
    font_monogram_data[270][ 6] = 0b0011101;
    font_monogram_data[270][ 7] = 0b0000101;
    font_monogram_data[270][ 8] = 0b0000101;
    font_monogram_data[270][ 9] = 0b0011110;
    font_monogram_chars[0x152] = 270;
    // 'œ'
    font_monogram_data[271][ 5] = 0b0001110;
    font_monogram_data[271][ 6] = 0b0010101;
    font_monogram_data[271][ 7] = 0b0011101;
    font_monogram_data[271][ 8] = 0b0000101;
    font_monogram_data[271][ 9] = 0b0001110;
    font_monogram_chars[0x153] = 271;
    // 'Ŕ'
    font_monogram_data[272][ 0] = 0b0001000;
    font_monogram_data[272][ 1] = 0b0000100;
    font_monogram_data[272][ 3] = 0b0001111;
    font_monogram_data[272][ 4] = 0b0010001;
    font_monogram_data[272][ 5] = 0b0010001;
    font_monogram_data[272][ 6] = 0b0001111;
    font_monogram_data[272][ 7] = 0b0010001;
    font_monogram_data[272][ 8] = 0b0010001;
    font_monogram_data[272][ 9] = 0b0010001;
    font_monogram_chars[0x154] = 272;
    // 'ŕ'
    font_monogram_data[273][ 2] = 0b0001000;
    font_monogram_data[273][ 3] = 0b0000100;
    font_monogram_data[273][ 5] = 0b0001101;
    font_monogram_data[273][ 6] = 0b0010011;
    font_monogram_data[273][ 7] = 0b0000001;
    font_monogram_data[273][ 8] = 0b0000001;
    font_monogram_data[273][ 9] = 0b0000001;
    font_monogram_chars[0x155] = 273;
    // 'Ŗ'
    font_monogram_data[274][ 3] = 0b0001111;
    font_monogram_data[274][ 4] = 0b0010001;
    font_monogram_data[274][ 5] = 0b0010001;
    font_monogram_data[274][ 6] = 0b0001111;
    font_monogram_data[274][ 7] = 0b0010001;
    font_monogram_data[274][ 8] = 0b0010001;
    font_monogram_data[274][ 9] = 0b0010001;
    font_monogram_data[274][10] = 0b0000100;
    font_monogram_data[274][11] = 0b0000011;
    font_monogram_chars[0x156] = 274;
    // 'ŗ'
    font_monogram_data[275][ 5] = 0b0001101;
    font_monogram_data[275][ 6] = 0b0010011;
    font_monogram_data[275][ 7] = 0b0000001;
    font_monogram_data[275][ 8] = 0b0000001;
    font_monogram_data[275][ 9] = 0b0000001;
    font_monogram_data[275][10] = 0b0000100;
    font_monogram_data[275][11] = 0b0000011;
    font_monogram_chars[0x157] = 275;
    // 'Ř'
    font_monogram_data[276][ 0] = 0b0001010;
    font_monogram_data[276][ 1] = 0b0000100;
    font_monogram_data[276][ 3] = 0b0001111;
    font_monogram_data[276][ 4] = 0b0010001;
    font_monogram_data[276][ 5] = 0b0010001;
    font_monogram_data[276][ 6] = 0b0001111;
    font_monogram_data[276][ 7] = 0b0010001;
    font_monogram_data[276][ 8] = 0b0010001;
    font_monogram_data[276][ 9] = 0b0010001;
    font_monogram_chars[0x158] = 276;
    // 'ř'
    font_monogram_data[277][ 2] = 0b0001010;
    font_monogram_data[277][ 3] = 0b0000100;
    font_monogram_data[277][ 5] = 0b0001101;
    font_monogram_data[277][ 6] = 0b0010011;
    font_monogram_data[277][ 7] = 0b0000001;
    font_monogram_data[277][ 8] = 0b0000001;
    font_monogram_data[277][ 9] = 0b0000001;
    font_monogram_chars[0x159] = 277;
    // 'Ś'
    font_monogram_data[278][ 0] = 0b0001000;
    font_monogram_data[278][ 1] = 0b0000100;
    font_monogram_data[278][ 3] = 0b0001110;
    font_monogram_data[278][ 4] = 0b0010001;
    font_monogram_data[278][ 5] = 0b0000001;
    font_monogram_data[278][ 6] = 0b0001110;
    font_monogram_data[278][ 7] = 0b0010000;
    font_monogram_data[278][ 8] = 0b0010001;
    font_monogram_data[278][ 9] = 0b0001110;
    font_monogram_chars[0x15a] = 278;
    // 'ś'
    font_monogram_data[279][ 2] = 0b0001000;
    font_monogram_data[279][ 3] = 0b0000100;
    font_monogram_data[279][ 5] = 0b0011110;
    font_monogram_data[279][ 6] = 0b0000001;
    font_monogram_data[279][ 7] = 0b0001110;
    font_monogram_data[279][ 8] = 0b0010000;
    font_monogram_data[279][ 9] = 0b0001111;
    font_monogram_chars[0x15b] = 279;
    // 'Ŝ'
    font_monogram_data[280][ 0] = 0b0000100;
    font_monogram_data[280][ 1] = 0b0001010;
    font_monogram_data[280][ 3] = 0b0001110;
    font_monogram_data[280][ 4] = 0b0010001;
    font_monogram_data[280][ 5] = 0b0000001;
    font_monogram_data[280][ 6] = 0b0001110;
    font_monogram_data[280][ 7] = 0b0010000;
    font_monogram_data[280][ 8] = 0b0010001;
    font_monogram_data[280][ 9] = 0b0001110;
    font_monogram_chars[0x15c] = 280;
    // 'ŝ'
    font_monogram_data[281][ 2] = 0b0000100;
    font_monogram_data[281][ 3] = 0b0001010;
    font_monogram_data[281][ 5] = 0b0011110;
    font_monogram_data[281][ 6] = 0b0000001;
    font_monogram_data[281][ 7] = 0b0001110;
    font_monogram_data[281][ 8] = 0b0010000;
    font_monogram_data[281][ 9] = 0b0001111;
    font_monogram_chars[0x15d] = 281;
    // 'Ş'
    font_monogram_data[282][ 3] = 0b0001110;
    font_monogram_data[282][ 4] = 0b0010001;
    font_monogram_data[282][ 5] = 0b0000001;
    font_monogram_data[282][ 6] = 0b0001110;
    font_monogram_data[282][ 7] = 0b0010000;
    font_monogram_data[282][ 8] = 0b0010001;
    font_monogram_data[282][ 9] = 0b0001110;
    font_monogram_data[282][10] = 0b0000100;
    font_monogram_data[282][11] = 0b0000011;
    font_monogram_chars[0x15e] = 282;
    // 'ş'
    font_monogram_data[283][ 5] = 0b0011110;
    font_monogram_data[283][ 6] = 0b0000001;
    font_monogram_data[283][ 7] = 0b0001110;
    font_monogram_data[283][ 8] = 0b0010000;
    font_monogram_data[283][ 9] = 0b0001111;
    font_monogram_data[283][10] = 0b0000100;
    font_monogram_data[283][11] = 0b0000011;
    font_monogram_chars[0x15f] = 283;
    // 'Š'
    font_monogram_data[284][ 0] = 0b0001010;
    font_monogram_data[284][ 1] = 0b0000100;
    font_monogram_data[284][ 3] = 0b0001110;
    font_monogram_data[284][ 4] = 0b0010001;
    font_monogram_data[284][ 5] = 0b0000001;
    font_monogram_data[284][ 6] = 0b0001110;
    font_monogram_data[284][ 7] = 0b0010000;
    font_monogram_data[284][ 8] = 0b0010001;
    font_monogram_data[284][ 9] = 0b0001110;
    font_monogram_chars[0x160] = 284;
    // 'š'
    font_monogram_data[285][ 2] = 0b0001010;
    font_monogram_data[285][ 3] = 0b0000100;
    font_monogram_data[285][ 5] = 0b0011110;
    font_monogram_data[285][ 6] = 0b0000001;
    font_monogram_data[285][ 7] = 0b0001110;
    font_monogram_data[285][ 8] = 0b0010000;
    font_monogram_data[285][ 9] = 0b0001111;
    font_monogram_chars[0x161] = 285;
    // 'Ţ'
    font_monogram_data[286][ 3] = 0b0011111;
    font_monogram_data[286][ 4] = 0b0000100;
    font_monogram_data[286][ 5] = 0b0000100;
    font_monogram_data[286][ 6] = 0b0000100;
    font_monogram_data[286][ 7] = 0b0000100;
    font_monogram_data[286][ 8] = 0b0000100;
    font_monogram_data[286][ 9] = 0b0000100;
    font_monogram_data[286][10] = 0b0000100;
    font_monogram_data[286][11] = 0b0000011;
    font_monogram_chars[0x162] = 286;
    // 'ţ'
    font_monogram_data[287][ 3] = 0b0000010;
    font_monogram_data[287][ 4] = 0b0000010;
    font_monogram_data[287][ 5] = 0b0001111;
    font_monogram_data[287][ 6] = 0b0000010;
    font_monogram_data[287][ 7] = 0b0000010;
    font_monogram_data[287][ 8] = 0b0000010;
    font_monogram_data[287][ 9] = 0b0011100;
    font_monogram_data[287][10] = 0b0001000;
    font_monogram_data[287][11] = 0b0000110;
    font_monogram_chars[0x163] = 287;
    // 'Ť'
    font_monogram_data[288][ 0] = 0b0001010;
    font_monogram_data[288][ 1] = 0b0000100;
    font_monogram_data[288][ 3] = 0b0011111;
    font_monogram_data[288][ 4] = 0b0000100;
    font_monogram_data[288][ 5] = 0b0000100;
    font_monogram_data[288][ 6] = 0b0000100;
    font_monogram_data[288][ 7] = 0b0000100;
    font_monogram_data[288][ 8] = 0b0000100;
    font_monogram_data[288][ 9] = 0b0000100;
    font_monogram_chars[0x164] = 288;
    // 'ť'
    font_monogram_data[289][ 2] = 0b0001000;
    font_monogram_data[289][ 3] = 0b0001010;
    font_monogram_data[289][ 4] = 0b0000010;
    font_monogram_data[289][ 5] = 0b0001111;
    font_monogram_data[289][ 6] = 0b0000010;
    font_monogram_data[289][ 7] = 0b0000010;
    font_monogram_data[289][ 8] = 0b0000010;
    font_monogram_data[289][ 9] = 0b0011100;
    font_monogram_chars[0x165] = 289;
    // 'Ŧ'
    font_monogram_data[290][ 3] = 0b0011111;
    font_monogram_data[290][ 4] = 0b0000100;
    font_monogram_data[290][ 5] = 0b0001110;
    font_monogram_data[290][ 6] = 0b0000100;
    font_monogram_data[290][ 7] = 0b0000100;
    font_monogram_data[290][ 8] = 0b0000100;
    font_monogram_data[290][ 9] = 0b0000100;
    font_monogram_chars[0x166] = 290;
    // 'ŧ'
    font_monogram_data[291][ 3] = 0b0000010;
    font_monogram_data[291][ 4] = 0b0001111;
    font_monogram_data[291][ 5] = 0b0000010;
    font_monogram_data[291][ 6] = 0b0001111;
    font_monogram_data[291][ 7] = 0b0000010;
    font_monogram_data[291][ 8] = 0b0000010;
    font_monogram_data[291][ 9] = 0b0011100;
    font_monogram_chars[0x167] = 291;
    // 'Ũ'
    font_monogram_data[292][ 0] = 0b0010110;
    font_monogram_data[292][ 1] = 0b0001001;
    font_monogram_data[292][ 3] = 0b0010001;
    font_monogram_data[292][ 4] = 0b0010001;
    font_monogram_data[292][ 5] = 0b0010001;
    font_monogram_data[292][ 6] = 0b0010001;
    font_monogram_data[292][ 7] = 0b0010001;
    font_monogram_data[292][ 8] = 0b0010001;
    font_monogram_data[292][ 9] = 0b0001110;
    font_monogram_chars[0x168] = 292;
    // 'ũ'
    font_monogram_data[293][ 2] = 0b0010110;
    font_monogram_data[293][ 3] = 0b0001001;
    font_monogram_data[293][ 5] = 0b0010001;
    font_monogram_data[293][ 6] = 0b0010001;
    font_monogram_data[293][ 7] = 0b0010001;
    font_monogram_data[293][ 8] = 0b0010001;
    font_monogram_data[293][ 9] = 0b0011110;
    font_monogram_chars[0x169] = 293;
    // 'Ū'
    font_monogram_data[294][ 1] = 0b0001110;
    font_monogram_data[294][ 3] = 0b0010001;
    font_monogram_data[294][ 4] = 0b0010001;
    font_monogram_data[294][ 5] = 0b0010001;
    font_monogram_data[294][ 6] = 0b0010001;
    font_monogram_data[294][ 7] = 0b0010001;
    font_monogram_data[294][ 8] = 0b0010001;
    font_monogram_data[294][ 9] = 0b0001110;
    font_monogram_chars[0x16a] = 294;
    // 'ū'
    font_monogram_data[295][ 3] = 0b0001110;
    font_monogram_data[295][ 5] = 0b0010001;
    font_monogram_data[295][ 6] = 0b0010001;
    font_monogram_data[295][ 7] = 0b0010001;
    font_monogram_data[295][ 8] = 0b0010001;
    font_monogram_data[295][ 9] = 0b0011110;
    font_monogram_chars[0x16b] = 295;
    // 'Ŭ'
    font_monogram_data[296][ 0] = 0b0001010;
    font_monogram_data[296][ 1] = 0b0000100;
    font_monogram_data[296][ 3] = 0b0010001;
    font_monogram_data[296][ 4] = 0b0010001;
    font_monogram_data[296][ 5] = 0b0010001;
    font_monogram_data[296][ 6] = 0b0010001;
    font_monogram_data[296][ 7] = 0b0010001;
    font_monogram_data[296][ 8] = 0b0010001;
    font_monogram_data[296][ 9] = 0b0001110;
    font_monogram_chars[0x16c] = 296;
    // 'ŭ'
    font_monogram_data[297][ 2] = 0b0001010;
    font_monogram_data[297][ 3] = 0b0000100;
    font_monogram_data[297][ 5] = 0b0010001;
    font_monogram_data[297][ 6] = 0b0010001;
    font_monogram_data[297][ 7] = 0b0010001;
    font_monogram_data[297][ 8] = 0b0010001;
    font_monogram_data[297][ 9] = 0b0011110;
    font_monogram_chars[0x16d] = 297;
    // 'Ů'
    font_monogram_data[298][ 0] = 0b0001010;
    font_monogram_data[298][ 1] = 0b0000100;
    font_monogram_data[298][ 3] = 0b0010001;
    font_monogram_data[298][ 4] = 0b0010001;
    font_monogram_data[298][ 5] = 0b0010001;
    font_monogram_data[298][ 6] = 0b0010001;
    font_monogram_data[298][ 7] = 0b0010001;
    font_monogram_data[298][ 8] = 0b0010001;
    font_monogram_data[298][ 9] = 0b0001110;
    font_monogram_chars[0x16e] = 298;
    // 'ů'
    font_monogram_data[299][ 1] = 0b0000100;
    font_monogram_data[299][ 2] = 0b0001010;
    font_monogram_data[299][ 3] = 0b0000100;
    font_monogram_data[299][ 5] = 0b0010001;
    font_monogram_data[299][ 6] = 0b0010001;
    font_monogram_data[299][ 7] = 0b0010001;
    font_monogram_data[299][ 8] = 0b0010001;
    font_monogram_data[299][ 9] = 0b0011110;
    font_monogram_chars[0x16f] = 299;
    // 'Ű'
    font_monogram_data[300][ 0] = 0b0010100;
    font_monogram_data[300][ 1] = 0b0001010;
    font_monogram_data[300][ 3] = 0b0010001;
    font_monogram_data[300][ 4] = 0b0010001;
    font_monogram_data[300][ 5] = 0b0010001;
    font_monogram_data[300][ 6] = 0b0010001;
    font_monogram_data[300][ 7] = 0b0010001;
    font_monogram_data[300][ 8] = 0b0010001;
    font_monogram_data[300][ 9] = 0b0001110;
    font_monogram_chars[0x170] = 300;
    // 'ű'
    font_monogram_data[301][ 2] = 0b0010100;
    font_monogram_data[301][ 3] = 0b0001010;
    font_monogram_data[301][ 5] = 0b0010001;
    font_monogram_data[301][ 6] = 0b0010001;
    font_monogram_data[301][ 7] = 0b0010001;
    font_monogram_data[301][ 8] = 0b0010001;
    font_monogram_data[301][ 9] = 0b0011110;
    font_monogram_chars[0x171] = 301;
    // 'Ų'
    font_monogram_data[302][ 3] = 0b0010001;
    font_monogram_data[302][ 4] = 0b0010001;
    font_monogram_data[302][ 5] = 0b0010001;
    font_monogram_data[302][ 6] = 0b0010001;
    font_monogram_data[302][ 7] = 0b0010001;
    font_monogram_data[302][ 8] = 0b0010001;
    font_monogram_data[302][ 9] = 0b0001110;
    font_monogram_data[302][10] = 0b0000100;
    font_monogram_data[302][11] = 0b0011000;
    font_monogram_chars[0x172] = 302;
    // 'ų'
    font_monogram_data[303][ 5] = 0b0010001;
    font_monogram_data[303][ 6] = 0b0010001;
    font_monogram_data[303][ 7] = 0b0010001;
    font_monogram_data[303][ 8] = 0b0010001;
    font_monogram_data[303][ 9] = 0b0011110;
    font_monogram_data[303][10] = 0b0000100;
    font_monogram_data[303][11] = 0b0011000;
    font_monogram_chars[0x173] = 303;
    // 'Ŵ'
    font_monogram_data[304][ 0] = 0b0000100;
    font_monogram_data[304][ 1] = 0b0001010;
    font_monogram_data[304][ 3] = 0b0010001;
    font_monogram_data[304][ 4] = 0b0010001;
    font_monogram_data[304][ 5] = 0b0010001;
    font_monogram_data[304][ 6] = 0b0010001;
    font_monogram_data[304][ 7] = 0b0010101;
    font_monogram_data[304][ 8] = 0b0011011;
    font_monogram_data[304][ 9] = 0b0010001;
    font_monogram_chars[0x174] = 304;
    // 'ŵ'
    font_monogram_data[305][ 2] = 0b0000100;
    font_monogram_data[305][ 3] = 0b0001010;
    font_monogram_data[305][ 5] = 0b0010001;
    font_monogram_data[305][ 6] = 0b0010001;
    font_monogram_data[305][ 7] = 0b0010101;
    font_monogram_data[305][ 8] = 0b0010101;
    font_monogram_data[305][ 9] = 0b0001010;
    font_monogram_chars[0x175] = 305;
    // 'Ŷ'
    font_monogram_data[306][ 0] = 0b0000100;
    font_monogram_data[306][ 1] = 0b0001010;
    font_monogram_data[306][ 3] = 0b0010001;
    font_monogram_data[306][ 4] = 0b0010001;
    font_monogram_data[306][ 5] = 0b0001010;
    font_monogram_data[306][ 6] = 0b0000100;
    font_monogram_data[306][ 7] = 0b0000100;
    font_monogram_data[306][ 8] = 0b0000100;
    font_monogram_data[306][ 9] = 0b0000100;
    font_monogram_chars[0x176] = 306;
    // 'ŷ'
    font_monogram_data[307][ 2] = 0b0000100;
    font_monogram_data[307][ 3] = 0b0001010;
    font_monogram_data[307][ 5] = 0b0010001;
    font_monogram_data[307][ 6] = 0b0010001;
    font_monogram_data[307][ 7] = 0b0010001;
    font_monogram_data[307][ 8] = 0b0010001;
    font_monogram_data[307][ 9] = 0b0011110;
    font_monogram_data[307][10] = 0b0010000;
    font_monogram_data[307][11] = 0b0001110;
    font_monogram_chars[0x177] = 307;
    // 'Ÿ'
    font_monogram_data[308][ 1] = 0b0001010;
    font_monogram_data[308][ 3] = 0b0010001;
    font_monogram_data[308][ 4] = 0b0010001;
    font_monogram_data[308][ 5] = 0b0001010;
    font_monogram_data[308][ 6] = 0b0000100;
    font_monogram_data[308][ 7] = 0b0000100;
    font_monogram_data[308][ 8] = 0b0000100;
    font_monogram_data[308][ 9] = 0b0000100;
    font_monogram_chars[0x178] = 308;
    // 'Ź'
    font_monogram_data[309][ 0] = 0b0001000;
    font_monogram_data[309][ 1] = 0b0000100;
    font_monogram_data[309][ 3] = 0b0011111;
    font_monogram_data[309][ 4] = 0b0010000;
    font_monogram_data[309][ 5] = 0b0001000;
    font_monogram_data[309][ 6] = 0b0000100;
    font_monogram_data[309][ 7] = 0b0000010;
    font_monogram_data[309][ 8] = 0b0000001;
    font_monogram_data[309][ 9] = 0b0011111;
    font_monogram_chars[0x179] = 309;
    // 'ź'
    font_monogram_data[310][ 2] = 0b0001000;
    font_monogram_data[310][ 3] = 0b0000100;
    font_monogram_data[310][ 5] = 0b0011111;
    font_monogram_data[310][ 6] = 0b0001000;
    font_monogram_data[310][ 7] = 0b0000100;
    font_monogram_data[310][ 8] = 0b0000010;
    font_monogram_data[310][ 9] = 0b0011111;
    font_monogram_chars[0x17a] = 310;
    // 'Ż'
    font_monogram_data[311][ 1] = 0b0000100;
    font_monogram_data[311][ 3] = 0b0011111;
    font_monogram_data[311][ 4] = 0b0010000;
    font_monogram_data[311][ 5] = 0b0001000;
    font_monogram_data[311][ 6] = 0b0000100;
    font_monogram_data[311][ 7] = 0b0000010;
    font_monogram_data[311][ 8] = 0b0000001;
    font_monogram_data[311][ 9] = 0b0011111;
    font_monogram_chars[0x17b] = 311;
    // 'ż'
    font_monogram_data[312][ 3] = 0b0000100;
    font_monogram_data[312][ 5] = 0b0011111;
    font_monogram_data[312][ 6] = 0b0001000;
    font_monogram_data[312][ 7] = 0b0000100;
    font_monogram_data[312][ 8] = 0b0000010;
    font_monogram_data[312][ 9] = 0b0011111;
    font_monogram_chars[0x17c] = 312;
    // 'Ž'
    font_monogram_data[313][ 0] = 0b0001010;
    font_monogram_data[313][ 1] = 0b0000100;
    font_monogram_data[313][ 3] = 0b0011111;
    font_monogram_data[313][ 4] = 0b0010000;
    font_monogram_data[313][ 5] = 0b0001000;
    font_monogram_data[313][ 6] = 0b0000100;
    font_monogram_data[313][ 7] = 0b0000010;
    font_monogram_data[313][ 8] = 0b0000001;
    font_monogram_data[313][ 9] = 0b0011111;
    font_monogram_chars[0x17d] = 313;
    // 'ž'
    font_monogram_data[314][ 2] = 0b0001010;
    font_monogram_data[314][ 3] = 0b0000100;
    font_monogram_data[314][ 5] = 0b0011111;
    font_monogram_data[314][ 6] = 0b0001000;
    font_monogram_data[314][ 7] = 0b0000100;
    font_monogram_data[314][ 8] = 0b0000010;
    font_monogram_data[314][ 9] = 0b0011111;
    font_monogram_chars[0x17e] = 314;
    // 'Ё'
    font_monogram_data[315][ 1] = 0b0001010;
    font_monogram_data[315][ 3] = 0b0011111;
    font_monogram_data[315][ 4] = 0b0000001;
    font_monogram_data[315][ 5] = 0b0000001;
    font_monogram_data[315][ 6] = 0b0001111;
    font_monogram_data[315][ 7] = 0b0000001;
    font_monogram_data[315][ 8] = 0b0000001;
    font_monogram_data[315][ 9] = 0b0011111;
    font_monogram_chars[0x401] = 315;
    // 'А'
    font_monogram_data[316][ 3] = 0b0001110;
    font_monogram_data[316][ 4] = 0b0010001;
    font_monogram_data[316][ 5] = 0b0010001;
    font_monogram_data[316][ 6] = 0b0010001;
    font_monogram_data[316][ 7] = 0b0011111;
    font_monogram_data[316][ 8] = 0b0010001;
    font_monogram_data[316][ 9] = 0b0010001;
    font_monogram_chars[0x410] = 316;
    // 'Б'
    font_monogram_data[317][ 3] = 0b0011111;
    font_monogram_data[317][ 4] = 0b0000001;
    font_monogram_data[317][ 5] = 0b0000001;
    font_monogram_data[317][ 6] = 0b0001111;
    font_monogram_data[317][ 7] = 0b0010001;
    font_monogram_data[317][ 8] = 0b0010001;
    font_monogram_data[317][ 9] = 0b0001111;
    font_monogram_chars[0x411] = 317;
    // 'В'
    font_monogram_data[318][ 3] = 0b0001111;
    font_monogram_data[318][ 4] = 0b0010001;
    font_monogram_data[318][ 5] = 0b0010001;
    font_monogram_data[318][ 6] = 0b0001111;
    font_monogram_data[318][ 7] = 0b0010001;
    font_monogram_data[318][ 8] = 0b0010001;
    font_monogram_data[318][ 9] = 0b0001111;
    font_monogram_chars[0x412] = 318;
    // 'Г'
    font_monogram_data[319][ 3] = 0b0011111;
    font_monogram_data[319][ 4] = 0b0000001;
    font_monogram_data[319][ 5] = 0b0000001;
    font_monogram_data[319][ 6] = 0b0000001;
    font_monogram_data[319][ 7] = 0b0000001;
    font_monogram_data[319][ 8] = 0b0000001;
    font_monogram_data[319][ 9] = 0b0000001;
    font_monogram_chars[0x413] = 319;
    // 'Д'
    font_monogram_data[320][ 3] = 0b0001100;
    font_monogram_data[320][ 4] = 0b0001010;
    font_monogram_data[320][ 5] = 0b0001010;
    font_monogram_data[320][ 6] = 0b0001010;
    font_monogram_data[320][ 7] = 0b0001010;
    font_monogram_data[320][ 8] = 0b0001010;
    font_monogram_data[320][ 9] = 0b0011111;
    font_monogram_data[320][10] = 0b0010001;
    font_monogram_chars[0x414] = 320;
    // 'Е'
    font_monogram_data[321][ 3] = 0b0011111;
    font_monogram_data[321][ 4] = 0b0000001;
    font_monogram_data[321][ 5] = 0b0000001;
    font_monogram_data[321][ 6] = 0b0001111;
    font_monogram_data[321][ 7] = 0b0000001;
    font_monogram_data[321][ 8] = 0b0000001;
    font_monogram_data[321][ 9] = 0b0011111;
    font_monogram_chars[0x415] = 321;
    // 'Ж'
    font_monogram_data[322][ 3] = 0b0010101;
    font_monogram_data[322][ 4] = 0b0010101;
    font_monogram_data[322][ 5] = 0b0010101;
    font_monogram_data[322][ 6] = 0b0001110;
    font_monogram_data[322][ 7] = 0b0010101;
    font_monogram_data[322][ 8] = 0b0010101;
    font_monogram_data[322][ 9] = 0b0010101;
    font_monogram_chars[0x416] = 322;
    // 'З'
    font_monogram_data[323][ 3] = 0b0001110;
    font_monogram_data[323][ 4] = 0b0010001;
    font_monogram_data[323][ 5] = 0b0010000;
    font_monogram_data[323][ 6] = 0b0001110;
    font_monogram_data[323][ 7] = 0b0010000;
    font_monogram_data[323][ 8] = 0b0010001;
    font_monogram_data[323][ 9] = 0b0001110;
    font_monogram_chars[0x417] = 323;
    // 'И'
    font_monogram_data[324][ 3] = 0b0010001;
    font_monogram_data[324][ 4] = 0b0010001;
    font_monogram_data[324][ 5] = 0b0011001;
    font_monogram_data[324][ 6] = 0b0010101;
    font_monogram_data[324][ 7] = 0b0010011;
    font_monogram_data[324][ 8] = 0b0010001;
    font_monogram_data[324][ 9] = 0b0010001;
    font_monogram_chars[0x418] = 324;
    // 'Й'
    font_monogram_data[325][ 1] = 0b0001010;
    font_monogram_data[325][ 2] = 0b0000100;
    font_monogram_data[325][ 3] = 0b0010001;
    font_monogram_data[325][ 4] = 0b0010001;
    font_monogram_data[325][ 5] = 0b0011001;
    font_monogram_data[325][ 6] = 0b0010101;
    font_monogram_data[325][ 7] = 0b0010011;
    font_monogram_data[325][ 8] = 0b0010001;
    font_monogram_data[325][ 9] = 0b0010001;
    font_monogram_chars[0x419] = 325;
    // 'К'
    font_monogram_data[326][ 3] = 0b0011001;
    font_monogram_data[326][ 4] = 0b0000101;
    font_monogram_data[326][ 5] = 0b0000101;
    font_monogram_data[326][ 6] = 0b0000011;
    font_monogram_data[326][ 7] = 0b0000101;
    font_monogram_data[326][ 8] = 0b0001001;
    font_monogram_data[326][ 9] = 0b0010001;
    font_monogram_chars[0x41a] = 326;
    // 'Л'
    font_monogram_data[327][ 3] = 0b0011110;
    font_monogram_data[327][ 4] = 0b0010010;
    font_monogram_data[327][ 5] = 0b0010010;
    font_monogram_data[327][ 6] = 0b0010010;
    font_monogram_data[327][ 7] = 0b0010010;
    font_monogram_data[327][ 8] = 0b0010010;
    font_monogram_data[327][ 9] = 0b0010001;
    font_monogram_chars[0x41b] = 327;
    // 'М'
    font_monogram_data[328][ 3] = 0b0010001;
    font_monogram_data[328][ 4] = 0b0011011;
    font_monogram_data[328][ 5] = 0b0010101;
    font_monogram_data[328][ 6] = 0b0010001;
    font_monogram_data[328][ 7] = 0b0010001;
    font_monogram_data[328][ 8] = 0b0010001;
    font_monogram_data[328][ 9] = 0b0010001;
    font_monogram_chars[0x41c] = 328;
    // 'Н'
    font_monogram_data[329][ 3] = 0b0010001;
    font_monogram_data[329][ 4] = 0b0010001;
    font_monogram_data[329][ 5] = 0b0010001;
    font_monogram_data[329][ 6] = 0b0011111;
    font_monogram_data[329][ 7] = 0b0010001;
    font_monogram_data[329][ 8] = 0b0010001;
    font_monogram_data[329][ 9] = 0b0010001;
    font_monogram_chars[0x41d] = 329;
    // 'О'
    font_monogram_data[330][ 3] = 0b0001110;
    font_monogram_data[330][ 4] = 0b0010001;
    font_monogram_data[330][ 5] = 0b0010001;
    font_monogram_data[330][ 6] = 0b0010001;
    font_monogram_data[330][ 7] = 0b0010001;
    font_monogram_data[330][ 8] = 0b0010001;
    font_monogram_data[330][ 9] = 0b0001110;
    font_monogram_chars[0x41e] = 330;
    // 'П'
    font_monogram_data[331][ 3] = 0b0011111;
    font_monogram_data[331][ 4] = 0b0010001;
    font_monogram_data[331][ 5] = 0b0010001;
    font_monogram_data[331][ 6] = 0b0010001;
    font_monogram_data[331][ 7] = 0b0010001;
    font_monogram_data[331][ 8] = 0b0010001;
    font_monogram_data[331][ 9] = 0b0010001;
    font_monogram_chars[0x41f] = 331;
    // 'Р'
    font_monogram_data[332][ 3] = 0b0001111;
    font_monogram_data[332][ 4] = 0b0010001;
    font_monogram_data[332][ 5] = 0b0010001;
    font_monogram_data[332][ 6] = 0b0001111;
    font_monogram_data[332][ 7] = 0b0000001;
    font_monogram_data[332][ 8] = 0b0000001;
    font_monogram_data[332][ 9] = 0b0000001;
    font_monogram_chars[0x420] = 332;
    // 'С'
    font_monogram_data[333][ 3] = 0b0001110;
    font_monogram_data[333][ 4] = 0b0010001;
    font_monogram_data[333][ 5] = 0b0000001;
    font_monogram_data[333][ 6] = 0b0000001;
    font_monogram_data[333][ 7] = 0b0000001;
    font_monogram_data[333][ 8] = 0b0010001;
    font_monogram_data[333][ 9] = 0b0001110;
    font_monogram_chars[0x421] = 333;
    // 'Т'
    font_monogram_data[334][ 3] = 0b0011111;
    font_monogram_data[334][ 4] = 0b0000100;
    font_monogram_data[334][ 5] = 0b0000100;
    font_monogram_data[334][ 6] = 0b0000100;
    font_monogram_data[334][ 7] = 0b0000100;
    font_monogram_data[334][ 8] = 0b0000100;
    font_monogram_data[334][ 9] = 0b0000100;
    font_monogram_chars[0x422] = 334;
    // 'У'
    font_monogram_data[335][ 3] = 0b0010001;
    font_monogram_data[335][ 4] = 0b0010001;
    font_monogram_data[335][ 5] = 0b0010001;
    font_monogram_data[335][ 6] = 0b0010001;
    font_monogram_data[335][ 7] = 0b0011110;
    font_monogram_data[335][ 8] = 0b0010000;
    font_monogram_data[335][ 9] = 0b0001110;
    font_monogram_chars[0x423] = 335;
    // 'Ф'
    font_monogram_data[336][ 3] = 0b0000100;
    font_monogram_data[336][ 4] = 0b0001110;
    font_monogram_data[336][ 5] = 0b0010101;
    font_monogram_data[336][ 6] = 0b0010101;
    font_monogram_data[336][ 7] = 0b0010101;
    font_monogram_data[336][ 8] = 0b0001110;
    font_monogram_data[336][ 9] = 0b0000100;
    font_monogram_chars[0x424] = 336;
    // 'Х'
    font_monogram_data[337][ 3] = 0b0010001;
    font_monogram_data[337][ 4] = 0b0010001;
    font_monogram_data[337][ 5] = 0b0001010;
    font_monogram_data[337][ 6] = 0b0000100;
    font_monogram_data[337][ 7] = 0b0001010;
    font_monogram_data[337][ 8] = 0b0010001;
    font_monogram_data[337][ 9] = 0b0010001;
    font_monogram_chars[0x425] = 337;
    // 'Ц'
    font_monogram_data[338][ 4] = 0b0001001;
    font_monogram_data[338][ 5] = 0b0001001;
    font_monogram_data[338][ 6] = 0b0001001;
    font_monogram_data[338][ 7] = 0b0001001;
    font_monogram_data[338][ 8] = 0b0001001;
    font_monogram_data[338][ 9] = 0b0011111;
    font_monogram_data[338][10] = 0b0010000;
    font_monogram_chars[0x426] = 338;
    // 'Ч'
    font_monogram_data[339][ 3] = 0b0010001;
    font_monogram_data[339][ 4] = 0b0010001;
    font_monogram_data[339][ 5] = 0b0010001;
    font_monogram_data[339][ 6] = 0b0011110;
    font_monogram_data[339][ 7] = 0b0010000;
    font_monogram_data[339][ 8] = 0b0010000;
    font_monogram_data[339][ 9] = 0b0010000;
    font_monogram_chars[0x427] = 339;
    // 'Ш'
    font_monogram_data[340][ 3] = 0b0010101;
    font_monogram_data[340][ 4] = 0b0010101;
    font_monogram_data[340][ 5] = 0b0010101;
    font_monogram_data[340][ 6] = 0b0010101;
    font_monogram_data[340][ 7] = 0b0010101;
    font_monogram_data[340][ 8] = 0b0010101;
    font_monogram_data[340][ 9] = 0b0011111;
    font_monogram_chars[0x428] = 340;
    // 'Щ'
    font_monogram_data[341][ 3] = 0b0010101;
    font_monogram_data[341][ 4] = 0b0010101;
    font_monogram_data[341][ 5] = 0b0010101;
    font_monogram_data[341][ 6] = 0b0010101;
    font_monogram_data[341][ 7] = 0b0010101;
    font_monogram_data[341][ 8] = 0b0010101;
    font_monogram_data[341][ 9] = 0b0011111;
    font_monogram_data[341][10] = 0b0010000;
    font_monogram_chars[0x429] = 341;
    // 'Ъ'
    font_monogram_data[342][ 4] = 0b0000011;
    font_monogram_data[342][ 5] = 0b0000010;
    font_monogram_data[342][ 6] = 0b0001110;
    font_monogram_data[342][ 7] = 0b0010010;
    font_monogram_data[342][ 8] = 0b0010010;
    font_monogram_data[342][ 9] = 0b0001110;
    font_monogram_chars[0x42a] = 342;
    // 'Ы'
    font_monogram_data[343][ 4] = 0b0010001;
    font_monogram_data[343][ 5] = 0b0010001;
    font_monogram_data[343][ 6] = 0b0010011;
    font_monogram_data[343][ 7] = 0b0010101;
    font_monogram_data[343][ 8] = 0b0010101;
    font_monogram_data[343][ 9] = 0b0010011;
    font_monogram_chars[0x42b] = 343;
    // 'Ь'
    font_monogram_data[344][ 4] = 0b0000001;
    font_monogram_data[344][ 5] = 0b0000001;
    font_monogram_data[344][ 6] = 0b0001111;
    font_monogram_data[344][ 7] = 0b0010001;
    font_monogram_data[344][ 8] = 0b0010001;
    font_monogram_data[344][ 9] = 0b0001111;
    font_monogram_chars[0x42c] = 344;
    // 'Э'
    font_monogram_data[345][ 3] = 0b0001110;
    font_monogram_data[345][ 4] = 0b0010001;
    font_monogram_data[345][ 5] = 0b0010000;
    font_monogram_data[345][ 6] = 0b0011100;
    font_monogram_data[345][ 7] = 0b0010000;
    font_monogram_data[345][ 8] = 0b0010001;
    font_monogram_data[345][ 9] = 0b0001110;
    font_monogram_chars[0x42d] = 345;
    // 'Ю'
    font_monogram_data[346][ 3] = 0b0001001;
    font_monogram_data[346][ 4] = 0b0010101;
    font_monogram_data[346][ 5] = 0b0010101;
    font_monogram_data[346][ 6] = 0b0010111;
    font_monogram_data[346][ 7] = 0b0010101;
    font_monogram_data[346][ 8] = 0b0010101;
    font_monogram_data[346][ 9] = 0b0001001;
    font_monogram_chars[0x42e] = 346;
    // 'Я'
    font_monogram_data[347][ 4] = 0b0011110;
    font_monogram_data[347][ 5] = 0b0010001;
    font_monogram_data[347][ 6] = 0b0010001;
    font_monogram_data[347][ 7] = 0b0011110;
    font_monogram_data[347][ 8] = 0b0010001;
    font_monogram_data[347][ 9] = 0b0010001;
    font_monogram_chars[0x42f] = 347;
    // 'а'
    font_monogram_data[348][ 5] = 0b0001110;
    font_monogram_data[348][ 6] = 0b0010000;
    font_monogram_data[348][ 7] = 0b0011110;
    font_monogram_data[348][ 8] = 0b0010001;
    font_monogram_data[348][ 9] = 0b0011110;
    font_monogram_chars[0x430] = 348;
    // 'б'
    font_monogram_data[349][ 3] = 0b0011110;
    font_monogram_data[349][ 4] = 0b0000001;
    font_monogram_data[349][ 5] = 0b0001101;
    font_monogram_data[349][ 6] = 0b0010011;
    font_monogram_data[349][ 7] = 0b0010001;
    font_monogram_data[349][ 8] = 0b0010001;
    font_monogram_data[349][ 9] = 0b0001110;
    font_monogram_chars[0x431] = 349;
    // 'в'
    font_monogram_data[350][ 5] = 0b0001111;
    font_monogram_data[350][ 6] = 0b0010001;
    font_monogram_data[350][ 7] = 0b0001111;
    font_monogram_data[350][ 8] = 0b0010001;
    font_monogram_data[350][ 9] = 0b0001111;
    font_monogram_chars[0x432] = 350;
    // 'г'
    font_monogram_data[351][ 5] = 0b0011111;
    font_monogram_data[351][ 6] = 0b0000001;
    font_monogram_data[351][ 7] = 0b0000001;
    font_monogram_data[351][ 8] = 0b0000001;
    font_monogram_data[351][ 9] = 0b0000001;
    font_monogram_chars[0x433] = 351;
    // 'д'
    font_monogram_data[352][ 5] = 0b0001100;
    font_monogram_data[352][ 6] = 0b0001010;
    font_monogram_data[352][ 7] = 0b0001010;
    font_monogram_data[352][ 8] = 0b0001010;
    font_monogram_data[352][ 9] = 0b0011111;
    font_monogram_data[352][10] = 0b0010001;
    font_monogram_chars[0x434] = 352;
    // 'е'
    font_monogram_data[353][ 5] = 0b0001110;
    font_monogram_data[353][ 6] = 0b0010001;
    font_monogram_data[353][ 7] = 0b0011111;
    font_monogram_data[353][ 8] = 0b0000001;
    font_monogram_data[353][ 9] = 0b0001110;
    font_monogram_chars[0x435] = 353;
    // 'ж'
    font_monogram_data[354][ 5] = 0b0010101;
    font_monogram_data[354][ 6] = 0b0001110;
    font_monogram_data[354][ 7] = 0b0000100;
    font_monogram_data[354][ 8] = 0b0001110;
    font_monogram_data[354][ 9] = 0b0010101;
    font_monogram_chars[0x436] = 354;
    // 'з'
    font_monogram_data[355][ 5] = 0b0000110;
    font_monogram_data[355][ 6] = 0b0001001;
    font_monogram_data[355][ 7] = 0b0000100;
    font_monogram_data[355][ 8] = 0b0001001;
    font_monogram_data[355][ 9] = 0b0000110;
    font_monogram_chars[0x437] = 355;
    // 'и'
    font_monogram_data[356][ 5] = 0b0010001;
    font_monogram_data[356][ 6] = 0b0011001;
    font_monogram_data[356][ 7] = 0b0010101;
    font_monogram_data[356][ 8] = 0b0010011;
    font_monogram_data[356][ 9] = 0b0010001;
    font_monogram_chars[0x438] = 356;
    // 'й'
    font_monogram_data[357][ 3] = 0b0001010;
    font_monogram_data[357][ 4] = 0b0000100;
    font_monogram_data[357][ 5] = 0b0010001;
    font_monogram_data[357][ 6] = 0b0011001;
    font_monogram_data[357][ 7] = 0b0010101;
    font_monogram_data[357][ 8] = 0b0010011;
    font_monogram_data[357][ 9] = 0b0010001;
    font_monogram_chars[0x439] = 357;
    // 'к'
    font_monogram_data[358][ 5] = 0b0010001;
    font_monogram_data[358][ 6] = 0b0001001;
    font_monogram_data[358][ 7] = 0b0000111;
    font_monogram_data[358][ 8] = 0b0001001;
    font_monogram_data[358][ 9] = 0b0010001;
    font_monogram_chars[0x43a] = 358;
    // 'л'
    font_monogram_data[359][ 5] = 0b0011110;
    font_monogram_data[359][ 6] = 0b0010010;
    font_monogram_data[359][ 7] = 0b0010010;
    font_monogram_data[359][ 8] = 0b0010010;
    font_monogram_data[359][ 9] = 0b0010001;
    font_monogram_chars[0x43b] = 359;
    // 'м'
    font_monogram_data[360][ 5] = 0b0010001;
    font_monogram_data[360][ 6] = 0b0011011;
    font_monogram_data[360][ 7] = 0b0010101;
    font_monogram_data[360][ 8] = 0b0010001;
    font_monogram_data[360][ 9] = 0b0010001;
    font_monogram_chars[0x43c] = 360;
    // 'н'
    font_monogram_data[361][ 5] = 0b0010001;
    font_monogram_data[361][ 6] = 0b0010001;
    font_monogram_data[361][ 7] = 0b0011111;
    font_monogram_data[361][ 8] = 0b0010001;
    font_monogram_data[361][ 9] = 0b0010001;
    font_monogram_chars[0x43d] = 361;
    // 'о'
    font_monogram_data[362][ 5] = 0b0001110;
    font_monogram_data[362][ 6] = 0b0010001;
    font_monogram_data[362][ 7] = 0b0010001;
    font_monogram_data[362][ 8] = 0b0010001;
    font_monogram_data[362][ 9] = 0b0001110;
    font_monogram_chars[0x43e] = 362;
    // 'п'
    font_monogram_data[363][ 5] = 0b0011111;
    font_monogram_data[363][ 6] = 0b0010001;
    font_monogram_data[363][ 7] = 0b0010001;
    font_monogram_data[363][ 8] = 0b0010001;
    font_monogram_data[363][ 9] = 0b0010001;
    font_monogram_chars[0x43f] = 363;
    // 'р'
    font_monogram_data[364][ 5] = 0b0001101;
    font_monogram_data[364][ 6] = 0b0010011;
    font_monogram_data[364][ 7] = 0b0010001;
    font_monogram_data[364][ 8] = 0b0010001;
    font_monogram_data[364][ 9] = 0b0001111;
    font_monogram_data[364][10] = 0b0000001;
    font_monogram_data[364][11] = 0b0000001;
    font_monogram_chars[0x440] = 364;
    // 'с'
    font_monogram_data[365][ 5] = 0b0001110;
    font_monogram_data[365][ 6] = 0b0010001;
    font_monogram_data[365][ 7] = 0b0000001;
    font_monogram_data[365][ 8] = 0b0010001;
    font_monogram_data[365][ 9] = 0b0001110;
    font_monogram_chars[0x441] = 365;
    // 'т'
    font_monogram_data[366][ 5] = 0b0011111;
    font_monogram_data[366][ 6] = 0b0000100;
    font_monogram_data[366][ 7] = 0b0000100;
    font_monogram_data[366][ 8] = 0b0000100;
    font_monogram_data[366][ 9] = 0b0000100;
    font_monogram_chars[0x442] = 366;
    // 'у'
    font_monogram_data[367][ 5] = 0b0010001;
    font_monogram_data[367][ 6] = 0b0010001;
    font_monogram_data[367][ 7] = 0b0010001;
    font_monogram_data[367][ 8] = 0b0010001;
    font_monogram_data[367][ 9] = 0b0011110;
    font_monogram_data[367][10] = 0b0010000;
    font_monogram_data[367][11] = 0b0001110;
    font_monogram_chars[0x443] = 367;
    // 'ф'
    font_monogram_data[368][ 3] = 0b0000100;
    font_monogram_data[368][ 4] = 0b0000100;
    font_monogram_data[368][ 5] = 0b0001110;
    font_monogram_data[368][ 6] = 0b0010101;
    font_monogram_data[368][ 7] = 0b0010101;
    font_monogram_data[368][ 8] = 0b0010101;
    font_monogram_data[368][ 9] = 0b0001110;
    font_monogram_data[368][10] = 0b0000100;
    font_monogram_data[368][11] = 0b0000100;
    font_monogram_chars[0x444] = 368;
    // 'х'
    font_monogram_data[369][ 5] = 0b0010001;
    font_monogram_data[369][ 6] = 0b0001010;
    font_monogram_data[369][ 7] = 0b0000100;
    font_monogram_data[369][ 8] = 0b0001010;
    font_monogram_data[369][ 9] = 0b0010001;
    font_monogram_chars[0x445] = 369;
    // 'ц'
    font_monogram_data[370][ 5] = 0b0001001;
    font_monogram_data[370][ 6] = 0b0001001;
    font_monogram_data[370][ 7] = 0b0001001;
    font_monogram_data[370][ 8] = 0b0001001;
    font_monogram_data[370][ 9] = 0b0011111;
    font_monogram_data[370][10] = 0b0010000;
    font_monogram_chars[0x446] = 370;
    // 'ч'
    font_monogram_data[371][ 5] = 0b0010001;
    font_monogram_data[371][ 6] = 0b0010001;
    font_monogram_data[371][ 7] = 0b0010001;
    font_monogram_data[371][ 8] = 0b0011110;
    font_monogram_data[371][ 9] = 0b0010000;
    font_monogram_chars[0x447] = 371;
    // 'ш'
    font_monogram_data[372][ 5] = 0b0010101;
    font_monogram_data[372][ 6] = 0b0010101;
    font_monogram_data[372][ 7] = 0b0010101;
    font_monogram_data[372][ 8] = 0b0010101;
    font_monogram_data[372][ 9] = 0b0011111;
    font_monogram_chars[0x448] = 372;
    // 'щ'
    font_monogram_data[373][ 5] = 0b0010101;
    font_monogram_data[373][ 6] = 0b0010101;
    font_monogram_data[373][ 7] = 0b0010101;
    font_monogram_data[373][ 8] = 0b0010101;
    font_monogram_data[373][ 9] = 0b0011111;
    font_monogram_data[373][10] = 0b0010000;
    font_monogram_chars[0x449] = 373;
    // 'ъ'
    font_monogram_data[374][ 5] = 0b0000011;
    font_monogram_data[374][ 6] = 0b0000010;
    font_monogram_data[374][ 7] = 0b0001110;
    font_monogram_data[374][ 8] = 0b0010010;
    font_monogram_data[374][ 9] = 0b0001110;
    font_monogram_chars[0x44a] = 374;
    // 'ы'
    font_monogram_data[375][ 5] = 0b0010001;
    font_monogram_data[375][ 6] = 0b0010001;
    font_monogram_data[375][ 7] = 0b0010011;
    font_monogram_data[375][ 8] = 0b0010101;
    font_monogram_data[375][ 9] = 0b0010011;
    font_monogram_chars[0x44b] = 375;
    // 'ь'
    font_monogram_data[376][ 5] = 0b0000001;
    font_monogram_data[376][ 6] = 0b0000001;
    font_monogram_data[376][ 7] = 0b0001111;
    font_monogram_data[376][ 8] = 0b0010001;
    font_monogram_data[376][ 9] = 0b0001111;
    font_monogram_chars[0x44c] = 376;
    // 'э'
    font_monogram_data[377][ 5] = 0b0001110;
    font_monogram_data[377][ 6] = 0b0010001;
    font_monogram_data[377][ 7] = 0b0011100;
    font_monogram_data[377][ 8] = 0b0010001;
    font_monogram_data[377][ 9] = 0b0001110;
    font_monogram_chars[0x44d] = 377;
    // 'ю'
    font_monogram_data[378][ 5] = 0b0001001;
    font_monogram_data[378][ 6] = 0b0010101;
    font_monogram_data[378][ 7] = 0b0010111;
    font_monogram_data[378][ 8] = 0b0010101;
    font_monogram_data[378][ 9] = 0b0001001;
    font_monogram_chars[0x44e] = 378;
    // 'я'
    font_monogram_data[379][ 5] = 0b0011110;
    font_monogram_data[379][ 6] = 0b0010001;
    font_monogram_data[379][ 7] = 0b0010001;
    font_monogram_data[379][ 8] = 0b0011110;
    font_monogram_data[379][ 9] = 0b0010001;
    font_monogram_chars[0x44f] = 379;
    // 'ё'
    font_monogram_data[380][ 3] = 0b0001010;
    font_monogram_data[380][ 5] = 0b0001110;
    font_monogram_data[380][ 6] = 0b0010001;
    font_monogram_data[380][ 7] = 0b0011111;
    font_monogram_data[380][ 8] = 0b0000001;
    font_monogram_data[380][ 9] = 0b0001110;
    font_monogram_chars[0x451] = 380;
    // '—'
    font_monogram_data[381][ 6] = 0b0011111;
    font_monogram_chars[0x2014] = 381;
    // '’'
    font_monogram_data[382][ 3] = 0b0001000;
    font_monogram_data[382][ 4] = 0b0000100;
    font_monogram_chars[0x2019] = 382;
    // '…'
    font_monogram_data[383][ 8] = 0b0010101;
    font_monogram_data[383][ 9] = 0b0010101;
    font_monogram_chars[0x2026] = 383;
    // '€'
    font_monogram_data[384][ 3] = 0b0001100;
    font_monogram_data[384][ 4] = 0b0010010;
    font_monogram_data[384][ 5] = 0b0000111;
    font_monogram_data[384][ 6] = 0b0000010;
    font_monogram_data[384][ 7] = 0b0000111;
    font_monogram_data[384][ 8] = 0b0010010;
    font_monogram_data[384][ 9] = 0b0001100;
    font_monogram_chars[0x20ac] = 384;
    // '←'
    font_monogram_data[385][ 5] = 0b0000100;
    font_monogram_data[385][ 6] = 0b0011110;
    font_monogram_data[385][ 7] = 0b0011111;
    font_monogram_data[385][ 8] = 0b0011110;
    font_monogram_data[385][ 9] = 0b0000100;
    font_monogram_chars[0x2190] = 385;
    // '↑'
    font_monogram_data[386][ 5] = 0b0000100;
    font_monogram_data[386][ 6] = 0b0001110;
    font_monogram_data[386][ 7] = 0b0011111;
    font_monogram_data[386][ 8] = 0b0001110;
    font_monogram_data[386][ 9] = 0b0001110;
    font_monogram_chars[0x2191] = 386;
    // '→'
    font_monogram_data[387][ 5] = 0b0000100;
    font_monogram_data[387][ 6] = 0b0001111;
    font_monogram_data[387][ 7] = 0b0011111;
    font_monogram_data[387][ 8] = 0b0001111;
    font_monogram_data[387][ 9] = 0b0000100;
    font_monogram_chars[0x2192] = 387;
    // '↓'
    font_monogram_data[388][ 5] = 0b0001110;
    font_monogram_data[388][ 6] = 0b0001110;
    font_monogram_data[388][ 7] = 0b0011111;
    font_monogram_data[388][ 8] = 0b0001110;
    font_monogram_data[388][ 9] = 0b0000100;
    font_monogram_chars[0x2193] = 388;
    // ' '
    font_monogram_chars[0x20] = 389;
}


void
draw_monogram_char(u8 ch, u32* dest, size_t dest_w, u64 dest_x, u64 dest_y, u32 color)
{
    u32* d = dest + dest_x + dest_w * dest_y;
    for (u64 y = 0; y < font_monogram_height; ++y) {
        u8 pixel_bits = font_monogram_data[ch][y];
        u64 x = 0;
        while (pixel_bits) {
            if (pixel_bits & 1) {
                *(d+x) = color;
            }
            ++x;
            pixel_bits = pixel_bits >> 1;
            // This is why the bits in the data are stored in reverse order.
            // We draw the pixels left-to-right, but shift them and pick
            // them off the byte right-to-left.
        }
        d = d + dest_w;
    }
}


size_t FRAME_WIDTH = 202;  // 20 + 26 * font_monogram_width;
size_t FRAME_HEIGHT = 200; // 20 + (font_monogram_number_of_characters / 26) * font_monogram_height;

// RGBA pixels: 202 * 200
u32 frame_buffer[40400];


void anim_callback()
{
    // Grey background.
    memset(frame_buffer, 0x7f, sizeof(frame_buffer));

    for (size_t ch = 0; ch < font_monogram_number_of_characters; ++ch) {
        u64 x = ch % 26 * font_monogram_width;
        u64 y = ch / 26 * font_monogram_height;
        // Drop shadow.
        draw_monogram_char(ch, frame_buffer, FRAME_WIDTH, 11 + x, 11 + y, 0x00000000);
        // Foreground font glyphs.
        draw_monogram_char(ch, frame_buffer, FRAME_WIDTH, 10 + x, 10 + y, 0x00FFFFFF);
    }
    window_draw_frame(0, frame_buffer);

    time_delay_cb(10, anim_callback);
}


void main()
{
    //print_i64(font_monogram_data[0][0]); print_endl();  // prints '0'...
    init_monogram_font_data();  // buggy!
    //print_i64(font_monogram_data[0][0]); print_endl();  // prints '132' (T_T)
    // at this point font_monogram_data[0][0] is 132 aka 0b10000100
    // this should not be the case

    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Monogram Font Example", 0);

    time_delay_cb(0, anim_callback);

    enable_event_loop();
}
