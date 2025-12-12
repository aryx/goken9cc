
#define VLONG_MAX    ~(1LL<<63)

typedef long long vlong;

vlong test() {
    // 8c -S should generate some

    
    return VLONG_MAX;
}
