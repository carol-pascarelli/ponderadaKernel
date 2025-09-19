#ifndef KEYBOARD_MAP_H
#define KEYBOARD_MAP_H

unsigned char keyboard_map[128] =
{
    0,  27, '1','2','3','4','5','6','7','8', /* 9 */
  '9','0','-','=', '\b', /* Backspace */
  '\t',         /* Tab */
  'q','w','e','r','t','y','u','i','o','p','[',']','\n', /* Enter key */
    0,          /* Control key */
  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,          /* Left shift */
  '\\','z','x','c','v','b','n','m',',','.','/',
    0,          /* Right shift */
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    /* etc. pode expandir depois */
};

#endif
