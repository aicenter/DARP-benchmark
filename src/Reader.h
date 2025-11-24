///////////////////////////////////////////////////////////
//  Reader.h
//  Implementation of the Class Reader
//  Created on:      23-Mar-2020 10:50:59 AM
//  Original author: Fido
///////////////////////////////////////////////////////////

#pragma once

#include <filesystem>

#include "DARP_instance.h"


template <typename N> class Reader
{

public:
	virtual ~Reader() = default;
	
	virtual DARP_instance<N> read(std::filesystem::path filepath) = 0;
};
