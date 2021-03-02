cmake_minimum_required(VERSION 3.0.0)
project(miniBT VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(miniBT main.cpp BitSet.cpp MySocket.cpp PeerAcceptor.cpp PeerLink.cpp PeerManager.cpp PieceRequest.cpp RateMeasure.cpp SelectReactor.cpp sha1.cpp TaskStorage.cpp TCPTracker.cpp TorrentFile.cpp TorrentParser.cpp TorrentTask.cpp TrackManager.cpp)

target_link_libraries(miniBT pthread curl)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
