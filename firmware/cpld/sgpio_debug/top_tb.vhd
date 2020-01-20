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

LIBRARY ieee;
USE ieee.std_logic_1164.ALL;
 
ENTITY top_tb IS
END top_tb;
 
ARCHITECTURE behavior OF top_tb IS 
 
    COMPONENT top
    PORT(
        HOST_DATA : INOUT  std_logic_vector(7 downto 0);
        HOST_CAPTURE : OUT std_logic;
        HOST_DISABLE : IN std_logic;
        HOST_DIRECTION : IN std_logic;
		  HOST_DECIM_SEL : IN std_logic_vector(2 downto 0);
        DA : IN  std_logic_vector(7 downto 0);
        DD : OUT  std_logic_vector(9 downto 0);
        CODEC_CLK : IN  std_logic;
        CODEC_X2_CLK : IN  std_logic
    );
    END COMPONENT;

    --Inputs
    signal DA : std_logic_vector(7 downto 0) := (others => '0');
    signal CODEC_CLK : std_logic := '0';
    signal CODEC_X2_CLK : std_logic := '0';
    signal HOST_DISABLE : std_logic := '1';
    signal HOST_DIRECTION : std_logic := '0';
	 signal HOST_DECIM_SEL : std_logic_vector(2 downto 0) := "010";
    
	--BiDirs
    signal HOST_DATA : std_logic_vector(7 downto 0);

 	--Outputs
    signal DD : std_logic_vector(9 downto 0);
    signal HOST_CAPTURE : std_logic;
    
begin
 
    uut: top PORT MAP (
        HOST_DATA => HOST_DATA,
        HOST_CAPTURE => HOST_CAPTURE,
        HOST_DISABLE => HOST_DISABLE,
        HOST_DIRECTION => HOST_DIRECTION,
		  HOST_DECIM_SEL => HOST_DECIM_SEL,
        DA => DA,
        DD => DD,
        CODEC_CLK => CODEC_CLK,
        CODEC_X2_CLK => CODEC_X2_CLK
    );

    clk_process :process
    begin
		CODEC_CLK <= '1';
        CODEC_X2_CLK <= '1';
		wait for 12.5 ns;
		CODEC_X2_CLK <= '0';
		wait for 12.5 ns;
        CODEC_CLK <= '0';
        CODEC_X2_CLK <= '1';
		wait for 12.5 ns;
		CODEC_X2_CLK <= '0';
        wait for 12.5 ns;
    end process;
 
    adc_proc: process
    begin
        wait until rising_edge(CODEC_CLK);
        wait for 9 ns;
        DA <= "00000000";
        
        wait until falling_edge(CODEC_CLK);
        wait for 9 ns;
        DA <= "00000001";
        
    end process;

    sgpio_proc: process
    begin
        HOST_DATA <= (others => 'Z');
        
        HOST_DIRECTION <= '0';
        HOST_DISABLE <= '1';

        wait for 135 ns;
        
        HOST_DISABLE <= '0';
        
        wait for 1000 ns;
        
        HOST_DISABLE <= '1';
        
        wait for 100 ns;
        
        HOST_DIRECTION <= '1';
        
        wait for 100 ns;
        
        HOST_DISABLE <= '0';
        
        for i in 0 to 10 loop
            HOST_DATA <= (others => '0');
            wait until rising_edge(CODEC_CLK) and HOST_CAPTURE = '1';
            
            HOST_DATA <= (others => '1');
            wait until rising_edge(CODEC_CLK) and HOST_CAPTURE = '1';
        end loop;
        
        wait;
    end process;

end;