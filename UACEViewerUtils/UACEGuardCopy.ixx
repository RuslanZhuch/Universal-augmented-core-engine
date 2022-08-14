module;
#include <atomic>
#include <optional>
#include <mutex>

#include <type_traits>

export module UACEGuardCopy;

template <typename T>
class RetWrapper
{
public:
	constexpr RetWrapper() noexcept = default;
	constexpr RetWrapper(const T& value) noexcept : val(value)
	{}
	constexpr T get() const noexcept
	{
		return this->val;
	}
private:
	T val{};
};

template<>
struct RetWrapper<void>
{

};

template <typename TVal, typename TRet>
class Mirror
{
public:
	using valueType = typename TVal;
	using retValueType = typename TRet;

	Mirror() = delete;
	explicit constexpr Mirror(TVal valPtr) noexcept
	{
		this->valPtr = valPtr;
	}

	Mirror(const Mirror&) = delete;
	Mirror(Mirror&& o) = delete;

	Mirror& operator=(const Mirror&) = delete;
	Mirror& operator=(Mirror&& o) = delete;

	void init() noexcept
	{
		this->afInUse.test_and_set(std::memory_order_acq_rel);
	}

	[[nodiscard]] constexpr auto getPtr() const noexcept
	{
		return this->valPtr;
	}

	[[nodiscard]] auto getInUse() const noexcept
	{
		return this->afInUse.test(std::memory_order_acquire);
	}

	void returnVal(RetWrapper<TRet> ret) noexcept requires std::is_object_v<TRet>
	{
		this->retValue = ret;
		this->afInUse.clear(std::memory_order_release);
	}

	void returnVal() noexcept requires std::is_same_v<TRet, void>
	{
		this->afInUse.clear(std::memory_order_release);
	}

	[[nodiscard]] constexpr auto getReturnVal() const noexcept requires std::is_object_v<TRet>
	{
		return this->retValue.get();
	}

private:
	TVal valPtr{ };
	std::atomic_flag afInUse = ATOMIC_FLAG_INIT;
	RetWrapper<TRet> retValue;

};

export namespace UACE::GuardCopy
{
	
	template <typename TMirror>
	class MirrorWrapper
	{
		using retValueType = typename TMirror::retValueType;
	public:
		MirrorWrapper(TMirror* mirror) noexcept
			:mirror(mirror)
		{
			if (this->mirror)
				this->mirror->init();
		}

		constexpr MirrorWrapper(MirrorWrapper&& o) noexcept
			: mirror(o.mirror)
		{}
		constexpr MirrorWrapper& operator=(MirrorWrapper&& o) noexcept
		{
			std::exchange(this->mirror, o.mirror);
		}

		MirrorWrapper(const MirrorWrapper&) = delete;
		MirrorWrapper& operator=(const MirrorWrapper&) = delete;

		~MirrorWrapper() noexcept
		{
			if (this->mirror)
			{
				if constexpr (std::is_object_v<retValueType>)
					this->mirror->returnVal({});
				else
					this->mirror->returnVal();
			}
		}

		[[nodiscard]] TMirror::valueType getPtr() const noexcept
		{
			if (this->mirror == nullptr)
				return {};
			if (!this->mirror->getInUse())
				return {};
			return this->mirror->getPtr();
		}

		void returnVal(const RetWrapper<retValueType>& retVal) noexcept requires std::is_object_v<retValueType>
		{
			if (this->mirror && this->mirror->getInUse())
				this->mirror->returnVal(retVal);
		}

		void returnVal() noexcept requires std::is_same_v<retValueType, void>
		{
			if (this->mirror)
				this->mirror->returnVal();
		}

	private:
		TMirror* mirror{ nullptr };
	};

	template <typename TVal, typename TRet = void>
	class Holder
	{
		using TMirror = Mirror<TVal, TRet>;

	public:
		explicit Holder(TVal valPtr) noexcept
			:mirror(valPtr)
		{}
		Holder(const Holder&) = delete;
		Holder& operator=(const Holder&) = delete;

		Holder(Holder&&) noexcept = default;
		Holder& operator=(Holder&&) noexcept = default;

		~Holder() noexcept = default;

		[[nodiscard]] MirrorWrapper<TMirror> createMirror() const noexcept
		{
			if (this->mirror.getInUse())
				return MirrorWrapper<TMirror>(nullptr);

			return MirrorWrapper<TMirror>(&this->mirror);
		}

		[[nodiscard]] bool getReturned() const noexcept requires std::is_same_v<TRet, void>
		{
			return !this->mirror.getInUse();
		}

		[[nodiscard]] std::optional<TRet> getReturned() const noexcept requires std::is_object_v<TRet>
		{
			if (this->mirror.getInUse())
				return std::nullopt;
			return this->mirror.getReturnVal();
		}

	private:
		mutable TMirror mirror;

	};

};