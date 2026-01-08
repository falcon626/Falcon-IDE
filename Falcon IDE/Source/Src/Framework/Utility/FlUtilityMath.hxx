
namespace Formula // Convenience Functions
{
	// Simple Rand Function
	[[nodiscard(L"Not Random Return Value")]] decltype(auto) Rand() noexcept
	{
		std::random_device rd;
		std::mt19937 mt{ rd() };

		std::uniform_int_distribution<> dist{ Def::IntZero, RAND_MAX };
		return dist(mt);
	}

	template<typename T> // Rand Function
	[[nodiscard(L"Not Random Return Value")]] decltype(auto) Rand
	(_In_ const T min, _In_ const std::type_identity_t<T> max = { std::numeric_limits<std::type_identity_t<T>>::max() }) noexcept
	{
		std::random_device rd;
		std::mt19937 mt{ rd() };

		if constexpr (std::is_integral<T>::value)
		{
			std::uniform_int_distribution<T> dist{ min, max };
			return dist(mt);
		}
		else if constexpr (std::is_floating_point<T>::value)
		{
			std::uniform_real_distribution<T> dist{ min, max };
			return dist(mt);
		}
		else static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "Not A Valid Numeric Type");
	}

	template<typename _T> // Rand Function With Exclusion Function
	[[nodiscard(L"Not Random Return Value")]] decltype(auto) Rand
	(_In_ const _T min, _In_ const std::type_identity_t<_T> max, _In_ const std::initializer_list<std::type_identity_t<_T>>& exclusion) noexcept
	{
		std::random_device rd;
		std::mt19937 mt{ rd() };

		// Check Exclusion Function (Lambda)
		auto isExcluded{ [&exclusion](const _T& value)
			{ return std::find(exclusion.begin(), exclusion.end(), value) != exclusion.end(); }
		};

		if constexpr (std::is_integral<_T>::value)
		{
			std::uniform_int_distribution<_T> dist{ min, max };
			_T value{};
			do {
				value = dist(mt);
			} while (isExcluded(value));
			return value;
		}
		else if constexpr (std::is_floating_point<_T>::value)
		{
			std::uniform_real_distribution<_T> dist{ min, max };
			_T value{};
			do {
				value = dist(mt);
			} while (isExcluded(value));
			return value;
		}
		else static_assert(std::is_integral<_T>::value || std::is_floating_point<_T>::value, "Not A Valid Numeric Type");
	}

	// Item <Name, Percentage> Return Item Name
	const std::string Lottery(_In_ const std::unordered_map<std::string, double>& items) noexcept
	{
		if (items.empty()) return std::string{ " " };

		// Create CDF
		std::vector<std::pair<std::string, double>> cdf;
		auto cumulative{ Def::DoubleZero };
		for (decltype(auto) item : items)
		{
			cumulative += item.second;
			cdf.emplace_back(item.first, cumulative);
		}

		auto randValue{ Rand(Def::DoubleZero,Def::DoubleOne) };

		// Select Item (Lambda)
		auto it{ std::lower_bound(cdf.begin(), cdf.end(), randValue,
			[](const std::pair<std::string, double>& element, double value)
			{ return element.second < value; }) };

		return it->first;
	}

	// Items Size == Percentages Size. Return ItemType
	template<typename ItemType, typename PercentageType>
	const auto Lottery(_In_ const std::vector<ItemType>& ids, _In_ const std::vector<PercentageType>& percentages) noexcept
	{
		if (ids.empty() || percentages.empty() || (ids.size() != percentages.size())) return static_cast<ItemType>(nullptr);
		std::unordered_map<ItemType, PercentageType> items;
		for (auto i{Def::UIntZero}; i < ids.size(); ++i)
			items.insert(ids[i], percentages[i]);

		// Create CDF
		std::vector<std::pair<ItemType, PercentageType>> cdf;
		auto cumulative{ percentages.empty() ? Def::DoubleZero : percentages[Def::UIntZero] };
		for (auto i{ Def::UIntZero }; i < percentages.size(); ++i)
		{
			cumulative += percentages[i];
			cdf.emplace_back(ids[i], cumulative);
		}

		PercentageType sun{};
		for (const auto& percentage : percentages)
			sun += percentage;

		auto randValue{ Rand<PercentageType>(Def::DoubleZero, sun) };

		// Select Item (Lambda)
		auto it{ std::upper_bound(cdf.begin(), cdf.end(), randValue,
			[](const std::pair<ItemType, PercentageType>& element, PercentageType value)
			{ return element.second <= value; }) };

		return it == cdf.end() ? static_cast<ItemType>(nullptr) : it->first;
	}

	inline auto SinCurve(const float degrees, const double amplitude = Def::DoubleOne, const double frequency = Def::DoubleOne, const double phase = Def::DoubleZero) noexcept 
	{
		const auto radians{ DirectX::XMConvertToRadians(degrees) };
		return amplitude * std::sin(frequency * radians + phase);
	}

	inline auto CosCurve(const float degrees, const double amplitude = Def::DoubleOne, const double frequency = Def::DoubleOne, const double phase = Def::DoubleZero) noexcept 
	{
		const auto radians{ DirectX::XMConvertToRadians(degrees) };
		return amplitude * std::cos(frequency * radians + phase);
	}

	inline auto TanCurve(const float degrees, const double amplitude = Def::DoubleOne, const double frequency = Def::DoubleOne, const double phase = Def::DoubleZero) noexcept 
	{
		const auto radians{ DirectX::XMConvertToRadians(degrees) };
		return amplitude * std::tan(frequency * radians + phase);
	}

	inline auto SinhCurve(const float degrees, const double amplitude = Def::DoubleOne, const double frequency = Def::DoubleOne, const double phase = Def::DoubleZero) noexcept
	{
		const auto radians{ DirectX::XMConvertToRadians(degrees) };
		return amplitude * std::sinh(frequency * radians + phase);
	}

	inline auto CoshCurve(const float degrees, const double amplitude = Def::DoubleOne, const double frequency = Def::DoubleOne, const double phase = Def::DoubleZero) noexcept
	{
		const auto radians{ DirectX::XMConvertToRadians(degrees) };
		return amplitude * std::cosh(frequency * radians + phase);
	}

	inline auto TanhCurve(const float degrees, const double amplitude = Def::DoubleOne, const double frequency = Def::DoubleOne, const double phase = Def::DoubleZero) noexcept
	{
		const auto radians{ DirectX::XMConvertToRadians(degrees) };
		return amplitude * std::tanh(frequency * radians + phase);
	}

	inline auto AsinCurve(const float degrees, const double amplitude = Def::DoubleOne, const double frequency = Def::DoubleOne, const double phase = Def::DoubleZero) noexcept
	{
		const auto radians{ DirectX::XMConvertToRadians(degrees) };
		return amplitude * std::asin(frequency * radians + phase);
	}

	inline auto AcosCurve(const float degrees, const double amplitude = Def::DoubleOne, const double frequency = Def::DoubleOne, const double phase = Def::DoubleZero) noexcept
	{
		const auto radians{ DirectX::XMConvertToRadians(degrees) };
		return amplitude * std::acos(frequency * radians + phase);
	}

	inline auto AtanCurve(const float degrees, const double amplitude = Def::DoubleOne, const double frequency = Def::DoubleOne, const double phase = Def::DoubleZero) noexcept
	{
		const auto radians{ DirectX::XMConvertToRadians(degrees) };
		return amplitude * std::atan(frequency * radians + phase);
	}

	inline auto GetDistance(const Math::Vector3& pos1, const Math::Vector3& pos2) noexcept
	{
		return std::sqrt((pos1.x - pos2.x) * (pos1.x - pos2.x) + (pos1.y - pos2.y) * (pos1.y - pos2.y) + (pos1.z - pos2.z) * (pos1.z - pos2.z));
	}

	inline auto GetDistance(const Math::Vector2& pos1, const Math::Vector2& pos2) noexcept
	{
		return std::sqrt((pos1.x - pos2.x) * (pos1.x - pos2.x) + (pos1.y - pos2.y) * (pos1.y - pos2.y));
	}

	inline auto GetDistance(const Math::Vector3& pos1, const Math::Vector2& pos2) noexcept
	{
		return std::sqrt((pos1.x - pos2.x) * (pos1.x - pos2.x) + (pos1.y - pos2.y) * (pos1.y - pos2.y) + (pos1.z - Def::FloatZero) * (pos1.z - Def::FloatZero));
	}

	inline auto GetDistance(const Math::Vector2& pos1, const Math::Vector3& pos2) noexcept
	{
		return std::sqrt((pos1.x - pos2.x) * (pos1.x - pos2.x) + (pos1.y - pos2.y) * (pos1.y - pos2.y) + (Def::FloatZero - pos2.z) * (Def::FloatZero - pos2.z));
	}
} // Name Space Formula