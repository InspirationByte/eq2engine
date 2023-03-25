//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Callable wrapper
//////////////////////////////////////////////////////////////////////////////////

#pragma once

static const size_t DEFAULT_FUNCTION_BUFFER_SIZE = 32;

template <typename, int = DEFAULT_FUNCTION_BUFFER_SIZE>
class EqFunction;

template <int BUFFER_SIZE, typename R, typename ...Args>
class EqFunction<R(Args...), BUFFER_SIZE>
{
public:
    ~EqFunction()
    {
        if (isSmall)
        {
            auto c = reinterpret_cast<Concept*>(&buffer);
            c->~Concept();
        }
        else
        {
            auto c = reinterpret_cast<std::unique_ptr<Concept>*>(&buffer);
            c->~unique_ptr();
        }
    }

    EqFunction() noexcept {};

    EqFunction(std::nullptr_t) noexcept {};

    EqFunction(EqFunction const& other)
    {
        if (!other)
            return;

        ASSERT((other.isSmall & 1) == other.isSmall);

        if (other.isSmall) 
        {
            isSmall = 1;
            auto c = reinterpret_cast<Concept const*>(&other.buffer);
            c->CopyToBuffer(&buffer);
        }
        else 
        {
            isSmall = 0;
            new (&buffer) std::unique_ptr<Concept>(other.ptr->Copy());
        }
        ASSERT((isSmall & 1) == isSmall);
    };

    EqFunction(EqFunction&& other) noexcept 
    {
        if (!other)
            return;
        ASSERT((other.isSmall & 1) == other.isSmall);

        MoveFunction(std::move(other));
    }

    template <typename F>
    EqFunction(F f)
    {
        if constexpr (std::is_pointer_v<F> || std::is_member_function_pointer_v<F>) 
        {
            // this will only be compiled if Func is a pointer type
            if (f == nullptr)
                return;
        }

        if constexpr (sizeof(f) <= BUFFER_SIZE && std::is_nothrow_move_constructible<F>::value) 
        {
            isSmall = 1;
            new (&buffer) Model<F>(std::move(f));

            ASSERT(isSmall == 1);
        }
        else 
        {
            isSmall = 0;
            new (&buffer) std::unique_ptr<Concept>(std::make_unique<Model<F>>(f));

            ASSERT(isSmall == 0);
        }
    }

    EqFunction& operator=(const EqFunction& other) 
    {
        EqFunction tmp(other);
        Swap(tmp);
        return *this;
    }

    EqFunction& operator=(EqFunction&& other) noexcept 
    {
        Cleanup();
        MoveFunction(std::move(other));
        return *this;
    }

    EqFunction& operator=(std::nullptr_t) noexcept
    {
        Cleanup();
        return *this;
    }

    void Swap(EqFunction& other) noexcept 
    {
        ASSERT((other.isSmall & 1) == other.isSmall);

        EqFunction tmp(std::move(other));
        other = std::move(*this);
        *this = std::move(tmp);

        ASSERT((isSmall & 1) == isSmall);
    }

    explicit operator bool() const noexcept 
    {
        return isSmall || ptr != nullptr;
    }

    R operator()(Args ... a) const 
    {
        ASSERT((isSmall & 1) == isSmall);

        if (isSmall) 
        {
            auto c = reinterpret_cast<Concept const*>(&buffer);
            return c->Invoke(std::forward<Args>(a)...);
        }
        else 
        {
            return ptr->Invoke(std::forward<Args>(a)...);
        }
    }

private:

    void Cleanup() 
    {
        if (isSmall)
        {
            auto c = reinterpret_cast<Concept*>(&buffer);
            c->~Concept();
        }
        else 
        {
            auto c = reinterpret_cast<std::unique_ptr<Concept>*>(&buffer);
            c->~unique_ptr();
        }
        memset(&buffer, 0, BUFFER_SIZE + alignof(size_t));
        isSmall = 0;
    }

    void MoveFunction(EqFunction&& other) 
    {
        ASSERT((other.isSmall & 1) == other.isSmall);

        if (other.isSmall) 
        {
            isSmall = 1;
            auto c = reinterpret_cast<Concept*>(&other.buffer);
            c->MoveToBuffer(&buffer);
            c->~Concept();
            other.isSmall = 0;
            new (&other.buffer) std::unique_ptr<Concept>(nullptr);
        }
        else 
        {
            isSmall = 0;
            new (&buffer) std::unique_ptr<Concept>(std::move(other.ptr));
        }

        ASSERT((isSmall & 1) == isSmall);
    }

    struct Concept 
    {
        virtual std::unique_ptr<Concept> Copy() const = 0;
        virtual R Invoke(Args&& ... a) const = 0;
        virtual ~Concept() = default;
        virtual void CopyToBuffer(void* buf) const = 0;
        virtual void MoveToBuffer(void* buf) noexcept = 0;
    };

    template <typename F>
    struct Model : Concept 
    {
        Model(F const& f) 
            : f(f) {}

        Model(F&& f) 
            : f(std::move(f)) {}

        std::unique_ptr<Concept> Copy() const override
        {
            return std::make_unique<Model<F>>(f);
        }

        R Invoke(Args&& ... a) const override 
        {
            return f(std::forward<Args>(a)...);
        }

        void CopyToBuffer(void* buf) const override 
        {
            new (buf) Model<F>(f);
        }

        void MoveToBuffer(void* buf) noexcept override
        {
            new (buf) Model<F>(std::move(f));
        }

        ~Model() override = default;

        F f;
    };

    int64 isSmall{ 0 };

    union
    {
        mutable std::aligned_storage_t<BUFFER_SIZE + alignof(size_t), alignof(size_t)> buffer{ 0 };
        std::unique_ptr<Concept> ptr;
    };
};

