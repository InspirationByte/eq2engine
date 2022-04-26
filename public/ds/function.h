//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Callable wrapper
//////////////////////////////////////////////////////////////////////////////////

#ifndef FUNCTION_H
#define FUNCTION_H

#include <stddef.h>
#include <memory>

template <typename>
struct EqFunction;

template <typename R, typename ...Args>
class EqFunction<R(Args...)>
{
public:
    EqFunction() noexcept : ptr(nullptr), isSmall(false) {};

    EqFunction(std::nullptr_t) noexcept : ptr(nullptr), isSmall(false) {};

    EqFunction(EqFunction const& other) : ptr(nullptr), isSmall(false) {
        if (!other) {
            return;
        }
        if (other.isSmall) {
            isSmall = true;
            auto c = reinterpret_cast<Concept const*>(&other.buffer);
            c->CopyToBuffer(&buffer);
        }
        else {
            isSmall = false;
            new (&buffer) std::unique_ptr<Concept>(other.ptr->Copy());
        }
    };

    EqFunction(EqFunction&& other) noexcept : ptr(nullptr), isSmall(false) {
        if (!other) {
            return;
        }
        MoveFunction(std::move(other));
    }

    template <typename F>
    EqFunction(F f) {
        if (sizeof(f) <= BUFFER_SIZE && std::is_nothrow_move_constructible<F>::value) {
            isSmall = true;
            new (&buffer) Model<F>(std::move(f));
        }
        else {
            isSmall = false;
            new (&buffer) std::unique_ptr<Concept>(std::make_unique<Model<F>>(f));
        }
    }

    ~EqFunction() {
        Cleanup();
    }

    EqFunction& operator=(const EqFunction& other) {
        EqFunction tmp(other);
        Swap(tmp);
        return *this;
    }

    EqFunction& operator=(EqFunction&& other) noexcept {
        Cleanup();
        MoveFunction(std::move(other));
        return *this;
    }

    void Swap(EqFunction& other) noexcept {
        EqFunction tmp(std::move(other));
        other = std::move(*this);
        *this = std::move(tmp);
    }

    explicit operator bool() const noexcept {
        return isSmall || ptr != nullptr;
    }

    R operator()(Args ... a) const {
        if (isSmall) {
            auto c = reinterpret_cast<Concept const*>(&buffer);
            return c->Invoke(std::forward<Args>(a)...);
        }
        else {
            return ptr->Invoke(std::forward<Args>(a)...);
        }
    }

private:

    void Cleanup() {
        if (isSmall) {
            auto c = reinterpret_cast<Concept*>(&buffer);
            c->~Concept();
        }
        else {
            auto c = reinterpret_cast<std::unique_ptr<Concept>*>(&buffer);
            c->~unique_ptr();
        }
        isSmall = false;
    }

    void MoveFunction(EqFunction&& other) {
        if (other.isSmall) {
            isSmall = true;
            auto c = reinterpret_cast<Concept*>(&other.buffer);
            c->MoveToBuffer(&buffer);
            c->~Concept();
            other.isSmall = false;
            new (&other.buffer) std::unique_ptr<Concept>(nullptr);
        }
        else {
            isSmall = false;
            new (&buffer) std::unique_ptr<Concept>(std::move(other.ptr));
        }
    }

    struct Concept {
        virtual std::unique_ptr<Concept> Copy() const = 0;
        virtual R Invoke(Args&& ... a) const = 0;
        virtual ~Concept() = default;
        virtual void CopyToBuffer(void* buf) const = 0;
        virtual void MoveToBuffer(void* buf) noexcept = 0;
    };


    template <typename F>
    struct Model : Concept {
        Model(F const& f) : f(f) {}

        Model(F&& f) : f(std::move(f)) {}

        std::unique_ptr<Concept> Copy() const override {
            return std::make_unique<Model<F>>(f);
        }

        R Invoke(Args&& ... a) const override {
            return f(std::forward<Args>(a)...);
        }

        void CopyToBuffer(void* buf) const override {
            new (buf) Model<F>(f);
        }

        void MoveToBuffer(void* buf) noexcept override {
            new (buf) Model<F>(std::move(f));
        }

        ~Model() override = default;

        F f;
    };

    static const size_t BUFFER_SIZE = 64;

    union {
        mutable std::aligned_storage_t<BUFFER_SIZE, alignof(size_t)> buffer;
        std::unique_ptr<Concept> ptr;
    };

    bool isSmall;
};

#endif