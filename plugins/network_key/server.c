/*
 * copyright: 2014
 * name : mhogo mchungu
 * email: mhogomchungu@gmail.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "network_key.h"

#define DEBUG 1

static void _debug( const char * msg )
{
#if DEBUG
	puts( msg ) ;
#endif
}

#define _cast( x ) ( const char * ) x

static zuluValue_t _get_key_from_wallet( const zuluKey_t * key )
{
	zuluValue_t value ;

	lxqt_wallet_t w ;

	lxqt_wallet_key_values_t key_value ;

	lxqt_wallet_error r ;

	/*
	 * make sure these are NULL terminated
	 */
	char wallet_name[ sizeof( key->wallet_name ) ] ;
	char application_name[ sizeof( key->wallet_name ) ] ;

	memcpy( wallet_name,key->wallet_name,sizeof( key->wallet_name ) ) ;
	memcpy( application_name,key->application_name,sizeof( key->application_name ) ) ;

	*( wallet_name + sizeof( key->wallet_name ) - 1 ) = '\0' ;
	*( application_name + sizeof( key->application_name ) - 1 ) = '\0' ;

	memset( &value,'\0',sizeof( zuluValue_t ) ) ;

	if( key->wallet_key_length > sizeof( key->wallet_key ) ||
		key->key_0_length > sizeof( key->key_0 ) ||
		key->key_1_length > sizeof( key->key_1 ) ){

		return value ;
	}else{
		r = lxqt_wallet_open( &w,key->wallet_key,key->wallet_key_length,wallet_name,application_name ) ;

		if( r == lxqt_wallet_no_error ){

			if( lxqt_wallet_read_key_value( w,key->key_0,key->key_0_length,&key_value ) ){

				if( key_value.key_value_size <= sizeof( value.value ) ){

					/*
					 * Try not to over flow our buffer.
					 */
					value.key_found    = 1 ;
					value.value_length = key_value.key_value_size ;
					memcpy( value.value,key_value.key_value,key_value.key_value_size ) ;
				}
			}else{
				if( lxqt_wallet_read_key_value( w,key->key_1,key->key_1_length,&key_value ) ){

					if( key_value.key_value_size <= sizeof( value.value ) ){

						/*
						 * Try not to over flow our buffer.
						 */
						value.key_found    = 1 ;
						value.value_length = key_value.key_value_size ;
						memcpy( value.value,key_value.key_value,key_value.key_value_size ) ;
					}
				}
			}

			lxqt_wallet_close( &w ) ;
		}

		return value ;
	}
}

static void _process_request( socket_t s,crypt_buffer_ctx ctx,const crypt_buffer_result * r )
{
	zuluValue_t value ;

	crypt_buffer_result e ;

	if( r->length != sizeof( zuluKey_t ) ){

		value.key_found = 0 ;
	}else{
		value = _get_key_from_wallet( r->buffer ) ;

		if( crypt_buffer_encrypt( ctx,_cast( &value ),sizeof( zuluValue_t ),&e ) ){

			SocketSendData( s,e.buffer,e.length ) ;
		}
	}
}

int main( int argc,char * argv[] )
{
	crypt_buffer_ctx ctx ;

	crypt_buffer_result r ;
	ssize_t n ;

	char * buffer = NULL ;

	char * encryption_key ;
	size_t encryption_key_length ;

	int network_port             = PORT_NUMBER ;
	const char * network_address = NETWORK_ADDRESS ;
	socket_t c ;

	char * e ;

	/*
	 * start a server at IP address 127.0.0.1 on port 2000
	 */
	socket_t s ;

	if( argc < 2 ){

		puts( "error: invalid number of arguments" ) ;
		return 1 ;
	}else if( argc >= 4 ){

		network_address = *( argv + 2 ) ;
		network_port    = atoi( *( argv + 3 ) ) ;
	}

	e = *( argv + 1 ) ;

	encryption_key_length = strlen( e ) ;

	encryption_key = malloc( sizeof( char ) * encryption_key_length ) ;
	memcpy( encryption_key,e,encryption_key_length ) ;

	memset( e,'\0',encryption_key_length ) ;

	s = SocketNet( network_address,network_port ) ;

	_debug( "server started" ) ;

	if( !SocketBind( s ) ){

		_debug( "socket bind failed" ) ;
		SocketClose( &s ) ;
		return 1 ;
	}

	if( !SocketListen( s ) ){

		_debug( "socket listen failed" ) ;
		SocketClose( &s ) ;
		return 1 ;
	}

	/*
	 * start decryption engine
	 */
	if( crypt_buffer_init( &ctx,encryption_key,encryption_key_length ) ){

		while( 1 ){
			/*
			 * accept client's connection
			 */
			c = SocketAccept( s ) ;

			buffer = NULL ;
			/*
			 * read network data client sent.
			 */
			n = SocketGetData( c,&buffer ) ;

			if( buffer ){

				if( crypt_buffer_decrypt( ctx,buffer,n,&r ) ){

					_process_request( c,ctx,&r ) ;
				}else{
					_debug( "failed to decrypt data" ) ;
				}

				/*
				 * free received data
				 */
				free( buffer ) ;
			}

			SocketClose( &c ) ;
		}

		/*
		 * shutdown decryption engine.
		 */
		crypt_buffer_uninit( &ctx ) ;
	}

	/*
	 * close used sockets
	 */
	SocketClose( &s ) ;

	memset( encryption_key,'\0',encryption_key_length ) ;
	free( e ) ;

	return 0 ;
}