#pragma once

class IJob {
public:
    virtual ~IJob() = default;
    virtual void Execute() = 0;

    std::chrono::high_resolution_clock::time_point m_RegisterTime;
};


template<typename Func>
class Job : public IJob {
public:
    Job(Func&& f) : m_Func(std::forward<Func>(f)) { m_RegisterTime = std::chrono::high_resolution_clock::now(); }
    void Execute() override {
        m_Func();
    }

private:
    Func m_Func;
};

struct JobComparer {
    bool operator()(std::shared_ptr<IJob> lhs, std::shared_ptr<IJob> rhs) const {
        return lhs->m_RegisterTime > rhs->m_RegisterTime;
    }
};