--
-- PostgreSQL database dump
--

-- Dumped from database version 14.13 (Ubuntu 14.13-0ubuntu0.22.04.1)
-- Dumped by pg_dump version 14.13 (Ubuntu 14.13-0ubuntu0.22.04.1)

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

--
-- Name: get_file_idx(text, text, text, bigint, bigint, bigint, text, bigint, bigint); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION public.get_file_idx(_host text, _path text, _filename text, _statlen bigint DEFAULT '-1'::integer, _reallen bigint DEFAULT '-1'::integer, _disk_usage bigint DEFAULT '-1'::integer, _hash text DEFAULT ''::text, _mtime bigint DEFAULT 0, _mtime_nsec bigint DEFAULT 0) RETURNS bigint
    LANGUAGE plpgsql
    AS $$
declare
	file_idx	bigint;
	dir_idx		bigint;

begin
	dir_idx = get_path_idx(_host, _path);

	select idx_files into file_idx from files
			where idx_dirs = dir_idx and filename = _filename;
	if not found
	then
		with idx_list as (
			insert into files (filename, idx_dirs, statlen, reallen, disk_usage, hash, mtime, mtime_nsec)
			values (_filename, dir_idx, _statlen, _reallen, _disk_usage, _hash, _mtime, _mtime_nsec)
			returning idx_files as idx_files
		)
		select idx_files into file_idx from idx_list;
	end if;

	return file_idx;
end;
$$;


ALTER FUNCTION public.get_file_idx(_host text, _path text, _filename text, _statlen bigint, _reallen bigint, _disk_usage bigint, _hash text, _mtime bigint, _mtime_nsec bigint) OWNER TO postgres;

--
-- Name: get_path_idx(text, text); Type: FUNCTION; Schema: public; Owner: olivier
--

CREATE FUNCTION public.get_path_idx(_host text, _path text) RETURNS bigint
    LANGUAGE plpgsql
    AS $$
declare
	idx bigint;

begin
select idx_dirs into idx from dirs where host = _host and path = _path;
if not found
then
	begin
		with idx_list as (
			insert into dirs (host, path) values (_host, _path) 
					returning idx_dirs as idx_dirs
		)
		select idx_dirs into idx from idx_list;
	end;
end if;

return idx;
end;
$$;


ALTER FUNCTION public.get_path_idx(_host text, _path text) OWNER TO olivier;

SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- Name: dirs; Type: TABLE; Schema: public; Owner: olivier
--

CREATE TABLE public.dirs (
    idx_dirs bigint NOT NULL,
    host text NOT NULL,
    path text NOT NULL,
    path_bak text
);


ALTER TABLE public.dirs OWNER TO olivier;

--
-- Name: dirs_idx_dirs_seq; Type: SEQUENCE; Schema: public; Owner: olivier
--

CREATE SEQUENCE public.dirs_idx_dirs_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.dirs_idx_dirs_seq OWNER TO olivier;

--
-- Name: dirs_idx_dirs_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: olivier
--

ALTER SEQUENCE public.dirs_idx_dirs_seq OWNED BY public.dirs.idx_dirs;


--
-- Name: files; Type: TABLE; Schema: public; Owner: olivier
--

CREATE TABLE public.files (
    idx_files bigint NOT NULL,
    filename text NOT NULL,
    idx_dirs bigint NOT NULL,
    statlen bigint DEFAULT '-1'::integer NOT NULL,
    reallen bigint DEFAULT '-1'::integer NOT NULL,
    disk_usage bigint NOT NULL,
    hash text DEFAULT ''::text NOT NULL,
    mtime bigint,
    mtime_nsec bigint,
    file_found boolean
);


ALTER TABLE public.files OWNER TO olivier;

--
-- Name: COLUMN files.file_found; Type: COMMENT; Schema: public; Owner: olivier
--

COMMENT ON COLUMN public.files.file_found IS 'Defaults to null, true if valid, false if bad';


--
-- Name: files_idx_files_seq; Type: SEQUENCE; Schema: public; Owner: olivier
--

CREATE SEQUENCE public.files_idx_files_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.files_idx_files_seq OWNER TO olivier;

--
-- Name: files_idx_files_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: olivier
--

ALTER SEQUENCE public.files_idx_files_seq OWNED BY public.files.idx_files;


--
-- Name: vfiles; Type: VIEW; Schema: public; Owner: postgres
--

CREATE VIEW public.vfiles AS
 SELECT d.idx_dirs,
    d.host,
    d.path,
    f.idx_files,
    f.filename,
    f.statlen,
    f.reallen,
    f.disk_usage,
    f.hash
   FROM (public.dirs d
     JOIN public.files f ON ((d.idx_dirs = f.idx_dirs)))
  ORDER BY d.host, d.path, f.filename;


ALTER TABLE public.vfiles OWNER TO postgres;

--
-- Name: dirs idx_dirs; Type: DEFAULT; Schema: public; Owner: olivier
--

ALTER TABLE ONLY public.dirs ALTER COLUMN idx_dirs SET DEFAULT nextval('public.dirs_idx_dirs_seq'::regclass);


--
-- Name: files idx_files; Type: DEFAULT; Schema: public; Owner: olivier
--

ALTER TABLE ONLY public.files ALTER COLUMN idx_files SET DEFAULT nextval('public.files_idx_files_seq'::regclass);


--
-- Name: dirs dirs_pkey; Type: CONSTRAINT; Schema: public; Owner: olivier
--

ALTER TABLE ONLY public.dirs
    ADD CONSTRAINT dirs_pkey PRIMARY KEY (idx_dirs);


--
-- Name: files files_pkey; Type: CONSTRAINT; Schema: public; Owner: olivier
--

ALTER TABLE ONLY public.files
    ADD CONSTRAINT files_pkey PRIMARY KEY (idx_files);


--
-- Name: dirs host_dir_uniq; Type: CONSTRAINT; Schema: public; Owner: olivier
--

ALTER TABLE ONLY public.dirs
    ADD CONSTRAINT host_dir_uniq UNIQUE (host, path);


--
-- Name: files path_filename_uniq; Type: CONSTRAINT; Schema: public; Owner: olivier
--

ALTER TABLE ONLY public.files
    ADD CONSTRAINT path_filename_uniq UNIQUE (idx_dirs, filename);


--
-- Name: fki_fk_dirs; Type: INDEX; Schema: public; Owner: olivier
--

CREATE INDEX fki_fk_dirs ON public.files USING btree (idx_dirs);


--
-- Name: hash_idx; Type: INDEX; Schema: public; Owner: olivier
--

CREATE INDEX hash_idx ON public.files USING btree (hash) WITH (deduplicate_items='true');


--
-- Name: path_dir_idx; Type: INDEX; Schema: public; Owner: olivier
--

CREATE INDEX path_dir_idx ON public.dirs USING btree (host, path) WITH (deduplicate_items='true');


--
-- Name: path_file_idx; Type: INDEX; Schema: public; Owner: olivier
--

CREATE INDEX path_file_idx ON public.files USING btree (idx_dirs, filename) WITH (deduplicate_items='true');


--
-- Name: realsize_path_name; Type: INDEX; Schema: public; Owner: olivier
--

CREATE INDEX realsize_path_name ON public.files USING btree (reallen, idx_files);


--
-- Name: size_path_name; Type: INDEX; Schema: public; Owner: olivier
--

CREATE INDEX size_path_name ON public.files USING btree (statlen, idx_files);


--
-- Name: files fk_dirs; Type: FK CONSTRAINT; Schema: public; Owner: olivier
--

ALTER TABLE ONLY public.files
    ADD CONSTRAINT fk_dirs FOREIGN KEY (idx_dirs) REFERENCES public.dirs(idx_dirs);


--
-- Name: FUNCTION get_file_idx(_host text, _path text, _filename text, _statlen bigint, _reallen bigint, _disk_usage bigint, _hash text, _mtime bigint, _mtime_nsec bigint); Type: ACL; Schema: public; Owner: postgres
--

GRANT ALL ON FUNCTION public.get_file_idx(_host text, _path text, _filename text, _statlen bigint, _reallen bigint, _disk_usage bigint, _hash text, _mtime bigint, _mtime_nsec bigint) TO olivier;


--
-- Name: TABLE vfiles; Type: ACL; Schema: public; Owner: postgres
--

GRANT ALL ON TABLE public.vfiles TO olivier;


--
-- PostgreSQL database dump complete
--

