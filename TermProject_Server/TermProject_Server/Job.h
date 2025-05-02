#pragma once

class IJob {
public:
    virtual ~IJob() = default;
    virtual void Execute() = 0;
};


template<typename Func>
class Job : public IJob {
public:
    Job(Func&& f) : m_Func(std::forward<Func>(f)) {}
    void Execute() override {
        m_Func();
    }
private:
    Func m_Func;
    bool m_ExecuteImmediately = true;
    std::chrono::high_resolution_clock::time_point m_ExecuteTime;
};

