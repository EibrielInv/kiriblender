/*
 * Copyright 2011, Blender Foundation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contributor: 
 *		Jeroen Bakker 
 *		Monique Dewanchand
 */

#ifndef _COM_Socket_h
#define _COM_Socket_h

#include <vector>
#include "BKE_text.h"
#include <string>
#include "DNA_node_types.h"
#include "COM_defines.h"

#ifdef WITH_CXX_GUARDEDALLOC
#include "MEM_guardedalloc.h"
#endif

using namespace std;
class SocketConnection;
class NodeBase;

/**
 * @brief Base class for InputSocket and OutputSocket.
 *
 * A socket are the points on an node where the user can make a connection between.
 * Sockets are always part of a node or an operation.
 *
 * @see InputSocket
 * @see OutputSocket
 * @see SocketConnection - a connection between an InputSocket and an OutputSocket
 * @ingroup Model
 */
class Socket {
private:
	/**
	 * Reference to the node where this Socket belongs to
	 */
	NodeBase *node;
	
	/**
	 * the datatype of this socket. Is used for automatically data transformation.
	 * @section data-conversion
	 */
	DataType datatype;
	
	bNodeSocket *editorSocket;
public:
	Socket(DataType datatype);
	
	DataType getDataType() const;
	void setNode(NodeBase *node);
	NodeBase *getNode() const;
	

	const virtual int isConnected() const;
	int isInputSocket() const;
	int isOutputSocket() const;
	virtual void determineResolution(int resolution[], unsigned int preferredResolution[]) {}

	void setEditorSocket(bNodeSocket *editorSocket) { this->editorSocket = editorSocket; }
	bNodeSocket *getbNodeSocket() const { return this->editorSocket; }
	
#ifdef WITH_CXX_GUARDEDALLOC
	MEM_CXX_CLASS_ALLOC_FUNCS("COM:Socket")
#endif
};


#endif
