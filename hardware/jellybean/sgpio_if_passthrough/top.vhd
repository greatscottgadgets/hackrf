--
-- Copyright 2012 Jared Boone
--
-- This file is part of HackRF.
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2, or (at your option)
-- any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; see the file COPYING.  If not, write to
-- the Free Software Foundation, Inc., 51 Franklin Street,
-- Boston, MA 02110-1301, USA.

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

library UNISIM;
use UNISIM.vcomponents.all;

entity top is
    Port(
        SGPIO           : inout std_logic_vector(15 downto 0);
           
        DA              : in    std_logic_vector(7 downto 0);
        DD              : out   std_logic_vector(9 downto 0);

        CODEC_CLK       : in    std_logic;
        CODEC_X2_CLK    : in    std_logic;
           
        B1AUX           : in    std_logic_vector(16 downto 9);
        B2AUX           : inout std_logic_vector(16 downto 1)
    );

end top;

architecture Behavioral of top is
    type transfer_direction is (to_sgpio, from_sgpio);
    signal transfer_direction_i : transfer_direction;

begin
    
    transfer_direction_i <= to_sgpio when B1AUX(9) = '0'
                                     else from_sgpio;

    DD <= (DD'high => '1', others => '0');
    
    B2AUX <= SGPIO when transfer_direction_i = from_sgpio
                   else (others => 'Z');

    SGPIO <= B2AUX when transfer_direction_i = to_sgpio
                   else (others => 'Z');
    
end Behavioral;
