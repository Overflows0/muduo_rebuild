

class noncopyable
{
public:
    noncopyable(const noncopyable &) = delete;
    void operator=(const noncopyable &) = delete;

private:
    noncopyable() = default;
    ~noncopyable() = default;
};