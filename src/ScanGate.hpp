#ifndef BRCODE_SCANGATE_HPP
#define BRCODE_SCANGATE_HPP
namespace brcode {
// Shared with the actual controller and portable tests. One acceptance per epoch.
class ScanGate {
public:
    ScanGate():epoch_(0),armed_(false),accepted_(false){}
    unsigned long begin(){++epoch_;armed_=true;accepted_=false;return epoch_;}
    void stop(){armed_=false;}
    bool accept(unsigned long epoch){if(!armed_||accepted_||epoch!=epoch_)return false;accepted_=true;armed_=false;return true;}
    unsigned long epoch()const{return epoch_;}
private:
    unsigned long epoch_;bool armed_,accepted_;
};
}
#endif
