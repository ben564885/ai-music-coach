-- Migration to rename mozart_raw_output column to audiveris_raw_output
-- Run this in your Supabase SQL Editor

ALTER TABLE sheet_music 
RENAME COLUMN mozart_raw_output TO audiveris_raw_output;

