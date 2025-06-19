#pragma once

#include <Windows.h>
#include <cinttypes>

void GetMachineId(uint8_t buffer[10]);

enum class SMBIOSStructType : uint8_t
{
	BIOSInformation = 0,
	SystemInformation = 1,
	BaseboardInformation = 2,
	SystemEnclosure = 3,
	ProcessorInformation = 4,
};

#pragma pack(push) 
#pragma pack(1)
struct SMBIOSHeader
{
	SMBIOSStructType Type;
	uint8_t Length;
	uint16_t Handle;
};

#pragma pack(push) 
#pragma pack(1)
class SMBIOSStruct
{
public:
	uint8_t GetLength()
	{
		return reinterpret_cast<SMBIOSHeader*>(this)->Length;
	}

	const char* GetString(uint8_t index)
	{
		if (index == 0) {
			return "";
		}

		--index;

		auto raw = reinterpret_cast<char*>(this);

		raw += GetLength();

		while (index--) {
			raw += strlen(raw) + 1;
		}

		return raw;
	}

	SMBIOSStructType GetType()
	{
		return reinterpret_cast<SMBIOSHeader*>(this)->Type;
	}

	bool IsType(SMBIOSStructType type)
	{
		return type == GetType();
	}

	SMBIOSStruct* Next();
};

#pragma pack(push) 
#pragma pack(1)
struct RawSMBIOSData
{
	uint8_t Used20CallingMethod;
	uint8_t MajorVersion;
	uint8_t MinorVersion;
	uint8_t DmiRevision;
	uint32_t Length;
	SMBIOSStruct SMBIOSTableData;

	SMBIOSStruct* Next(SMBIOSStruct* current = nullptr)
	{
		if (current == nullptr) {
			return &SMBIOSTableData;
		}

		auto next = current->Next();

		return reinterpret_cast<uint8_t*>(next) >= reinterpret_cast<uint8_t*>(&SMBIOSTableData) + Length ? nullptr : next;
	}
};

enum class SMBIOSBaseboardType : uint8_t
{
	Unknown = 0x01,
	Motherboard = 0x0a,
};

enum class SMBIOSBaseboardFeatureFlags : uint8_t
{
	HostingBoard = 0x01,
};

#pragma pack(push) 
#pragma pack(1)
// ReSharper disable once CppClassNeedsConstructorBecauseOfUninitializedMember
class SMBIOSBaseboard : public SMBIOSStruct
{
	uint32_t Header;
	uint8_t	Manufacturer;
	uint8_t	Product;
	uint8_t	Version;
	uint8_t	SerialNumber;
	uint8_t	AssetTag;
	SMBIOSBaseboardFeatureFlags	FeatureFlags;
	uint8_t	LocationInChassis;
	uint16_t ChassisHandle;
	SMBIOSBaseboardType	BoardType;
	uint8_t	ContainedObjectsLength;
	uint16_t *ContainedObjects;

public:
	const char* GetManufacturer()
	{
		return GetString(Manufacturer);
	}

	const char* GetProduct()
	{
		return GetString(Product);
	}

	const char* GetVersion()
	{
		return GetString(Version);
	}

	const char* GetSerialNumber()
	{
		return GetString(SerialNumber);
	}

	const char* GetAssetTag()
	{
		return GetString(AssetTag);
	}

	SMBIOSBaseboardFeatureFlags GetFeatureFlags() const
	{
		return FeatureFlags;
	}

	SMBIOSBaseboardType GetBoardType() const
	{
		return BoardType;
	}
};

#pragma pack(push) 
#pragma pack(1)
// ReSharper disable once CppClassNeedsConstructorBecauseOfUninitializedMember
class SMBIOSSystem : public SMBIOSStruct
{
	uint32_t Header;
	uint8_t	Manufacturer;
	uint8_t	ProductName;
	uint8_t	Version;
	uint8_t	SerialNumber;
	UUID Uuid;

public:
	const char* GetManufacturer()
	{
		return GetString(Manufacturer);
	}

	const char* GetProductName()
	{
		return GetString(ProductName);
	}

	const char* GetVersion()
	{
		return GetString(Version);
	}

	const char* GetSerialNumber()
	{
		return GetString(SerialNumber);
	}

	UUID GetUuid() const
	{
		return Uuid;
	}
};

enum class SMBIOSProcessorType : uint8_t
{
	Other = 1,
	Unknown = 2,
	CentralProcessor = 3,
	MathProcessor = 4,
	DSPProcessor = 5,
	VideoProcessor = 6,
};

#pragma pack(push) 
#pragma pack(1)
// ReSharper disable once CppClassNeedsConstructorBecauseOfUninitializedMember
class SMBIOSProcessor : public SMBIOSStruct
{
	uint32_t Header;
	uint8_t	SocketDesignation;
	SMBIOSProcessorType	ProcessorType;
	uint8_t	ProcessorFamily;
	uint8_t	ProcessorManufacturer;
	uint64_t ProcessorId; // Random on modern CPUS!
	uint8_t ProcessorVersion;
	uint8_t Voltage;
	uint16_t ExternalClock;
	uint16_t MaxSpeed;
	uint16_t CurrentSpeed;
	uint8_t Status;
	uint8_t ProcessorUpgrade;

	// 2.1+ (Length > 0x1A)
	uint16_t L1CacheHandle;
	uint16_t L2CacheHandle;
	uint16_t L3CacheHandle;

	// 3.2+ (Length > 0x23)
	uint8_t SerialNumber;
	uint8_t AssetTag;
	uint8_t PartNumber;


public:
	SMBIOSProcessorType GetProcessorType() const
	{
		return ProcessorType;
	}

	uint8_t GetProcessorFamily() const
	{
		return ProcessorFamily;
	}

	const char* GetSocketDesignation()
	{
		return GetString(SocketDesignation);
	}

	const char* GetProcessorManufacturer()
	{
		return GetString(ProcessorManufacturer);
	}

	const char* GetProcessorVersion()
	{
		return GetString(ProcessorVersion);
	}

	const char* GetSerialNumber()
	{
		return GetLength() > 0x23 ? GetString(SerialNumber) : "";
	}

	const char* GetAssetTag()
	{
		return GetLength() > 0x23 ? GetString(AssetTag) : "";
	}

	const char* GetPartNumber()
	{
		return GetLength() > 0x23 ? GetString(PartNumber) : "";
	}
};
