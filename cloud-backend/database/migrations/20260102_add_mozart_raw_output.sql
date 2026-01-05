-- Migration to add mozart_raw_output column to sheet_music table
-- Run this in your Supabase SQL Editor

ALTER TABLE sheet_music 
ADD COLUMN IF NOT EXISTS mozart_raw_output TEXT;

