#ifndef m5_display_bool_h
#define m5_display_bool.h
#ifdef M5STICK


// Custom transform that consumes a boolean value
// and displays it on the M5StickC screen
class M5DisplayBool : public BooleanTransform {

   public:
     M5DisplayBool() : BooleanTransform() {};

     virtual void set_input(bool value, uint8_t input_channel = 0) override;
};


#endif
#endif
