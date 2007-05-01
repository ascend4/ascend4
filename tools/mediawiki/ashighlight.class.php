<?php

define(ASHIGHLIGHT_LANG_ROOT,"/usr/share/highlight/langDefs");

class ASHighlight{

	function __construct($dir="/tmp",$default_lang="py"){
		$this->env=array();
		$this->dir=$dir;
		$this->encoding="ascii";	
		$this->line_numbers=false;
		$this->tabwidth=0;//zero means that we haven't specified it
		$this->default_lang=$default_lang;
	}

	function set_encoding($encoding="UTF-8"){
		$this->encoding=$encoding;
	}

	function enable_line_numbers(){
		$this->line_numbers = true;
	}
	function disable_line_numbers(){
		$this->line_numbers = false;
	}

	function start_line_numbers_at($start_line){
		$this->start_line = floor($start_line);
	}

	function set_tab_width($tabwidth){
		$this->tabwidth=floor($tabwidth);
	}

	function parse_code($text,$lang=""){
		$descriptorspec = array(
		   0 => array("pipe", "r"),  // stdin is a pipe that the child will read from
		   1 => array("pipe", "w"),  // stdout is a pipe that the child will write to
		   2 => array("pipe", "w")   // stderr is another pipe that the child (might) write to
		);

		if(!$lang)$lang=$this->default_lang;

		$cmd = "highlight --fragment"
			." --syntax=".escapeshellarg($lang);

		if($this->line_numbers){
			$cmd.=" --linenumbers";
			if(isset($this->start_line)){
				$cmd.=" --line-number-start=".$this->start_line;
			}
		}

		// only if the tabwidth value is non-zero will this
		// flag be shown
		if($this->tabwidth)$cmd.=" --replace-tabs=".$this->tabwidth;

		$css = $this->dir . "/" . "highlight.css";
		$cmd.=" --style-outfile=".escapeshellarg($css);
		if(file_exists($css)){
			$this->error=-888;
			$this->errmsg="'highlight.css' file already exists in ".$this->dir;
		}

		if(!$this->env)$this->env=array();

		$process = proc_open($cmd, $descriptorspec, $pipes, $this->dir, $this->env);

		if(is_resource($process)) {
		    // $pipes now looks like this:
		    // 0 => writeable handle connected to child stdin
		    // 1 => readable handle connected to child stdout
		    // Any error output will be appended to /tmp/error-output.txt

		    fwrite($pipes[0], $text);
		    fclose($pipes[0]);

		    $out = stream_get_contents($pipes[1]);
		    fclose($pipes[1]);

			$err = stream_get_contents($pipes[2]);
			fclose($pipes[2]);		

		    // It is important that you close any pipes before calling
		    // proc_close in order to avoid a deadlock
		    $this->error = proc_close($process);

			if(!$this->error){
				if(file_exists($css)){
					$this->stylesheet = file_get_contents($css);
					unlink($css);
					return $out;
				}else{
					$this->error=-777;
					$this->stylesheet="kwa{font:bold}";
					$this->errmsg="'$css' was not created by $cmd (dir=$this->dir)";
					return $out;
				}
			}else{
				$this->errmsg = $err;
			}
		}else{
			$this->error = -999;
			$this->errmsg="Process '$cmd' failed to start?";
		}
	}

	function get_stylesheet(){
		return $this->stylesheet;
	}		
}

?>
