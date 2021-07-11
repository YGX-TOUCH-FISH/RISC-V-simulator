//
// Created by 18163 on 2021/7/5.
//

#ifndef CODE_PRE_HPP
#define CODE_PRE_HPP
class Pre {
private:
    unsigned status = 0;//PHT
public:
    unsigned BRANCH = 0;
    unsigned SUCCESS = 0;
    Pre() = default;
    ~Pre() = default;
    void update(bool result) {
        switch (status) {
            case 0: {//强不发生
                if (!result) status = 1;
                break;
            }
            case 1: {//弱不发生
                if (!result) status = 3;
                else status = 0;
                break;
            }
            case 2: {//弱发生
                if (!result) status = 0;
                else status = 3;
                break;
            }
            case 3: {//强发生
                if (!result) status = 2;
                break;
            }
        }
    }
    bool tell() const{
        if (status == 3 || status == 2 ) return true;
        else return false;
    }
    bool match(bool result) const{
        return tell() == result;
    }
};
#endif //CODE_PRE_HPP
