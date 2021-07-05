//
// Created by 18163 on 2021/7/5.
//

#ifndef CODE_PRIDICTOR_HPP
#define CODE_PRIDICTOR_HPP
class Predictor {
private:
    unsigned status = 0;
public:
    Predictor() = default;
    ~Predictor() = default;
    void update(bool result) {
        switch (status) {
            case 0: {
                if (result) status = 1;
                break;
            }
            case 1: {
                if (result) status = 3;
                break;
            }
            case 2: {
                if (!result) status = 0;
                break;
            }
            case 3: {
                if (!result) status = 2;
                break;
            }
        }
    }
    bool tell() const{
        if (status == 3 || status == 2 ) return true;
        else return false;
    }
};
#endif //CODE_PRIDICTOR_HPP
