#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2001,2002  Josh Coalson
#
#  This program is part of FLAC; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

if FLaC__HAS_XMMS
XMMS_DIRS = plugin_xmms
endif

if FLaC__HAS_OGG
OGGFLAC_DIRS = libOggFLAC libOggFLAC++
OGGFLAC_TEST_DIRS = test_libOggFLAC test_libOggFLAC++
endif

SUBDIRS = \
	libFLAC \
	libFLAC++ \
	$(OGGFLAC_DIRS) \
	share \
	flac \
	metaflac \
	monkeys_audio_utilities \
	plugin_common \
	$(XMMS_DIRS) \
	plugin_winamp2 \
	plugin_winamp3 \
	test_libFLAC \
	test_libFLAC++ \
	$(OGGFLAC_TEST_DIRS) \
	test_streams
