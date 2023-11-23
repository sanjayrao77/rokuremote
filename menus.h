
int print_menus(struct userstate *uo);
int handlekey_menus(int *isquit_out, int *nextkey_out, struct userstate *userstate, struct discover *d, int ch);
int handlebutton_menus(struct userstate *userstate, struct discover *d, int code, struct joystick *js);
int repeat_handlebutton_menus(struct userstate *userstate, struct discover *d, int code);
void voidinit_buttons_menus(int *isfound_out, struct userstate *userstate);
