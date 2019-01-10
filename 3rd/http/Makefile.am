noinst_LIBRARIES = libbubi_http.a
libbubi_http_a_CXXFLAGS = -std=c++11 \
                         -DASIO_STANDALONE \
                         -I$(top_srcdir)/src/  \
                         -I$(top_srcdir)/src/3rd/protobuf/src/	\
                         -I$(top_srcdir)/src/3rd/jsoncpp/include/ \
                         -I$(top_srcdir)/src/3rd/asio/include \
						-I$(top_srcdir)/src/3rd/openssl/include \
						-I$(top_srcdir)/src/3rd/bzip2-1.0.6 \
						-I$(top_srcdir)/src/3rd/zlib-1.2.8 \
                         $(NULL)

libbubi_http_a_SOURCES = connection.cpp connection_manager.cpp HttpClient.cpp reply.cpp request_parser.cpp server.cpp io_service_pool.cpp

bubiconfigdir             = /etc/bubiconfig
							





