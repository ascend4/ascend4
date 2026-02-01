<?php

use MediaWiki\Shell\Shell;

class ASHighlight {
	private $binary;
	private $langRoot;
	private $timeout;
	private $tempDir;

	private $encoding = 'UTF-8';
	private $lineNumbers = false;
	private $tabWidth = 0;
	private $startLine = null;
	private $defaultLang = 'py';

	private $stylesheet = '';
	public $error = 0;
	public $errorMessage = '';

	public function __construct( array $options ) {
		$this->binary = $options['binary'];
		$this->langRoot = $options['langRoot'];
		$this->timeout = (int)$options['timeout'];
		$this->tempDir = $options['tempDir'];
	}

	public function setEncoding( $encoding ) {
		$this->encoding = $encoding;
	}

	public function enableLineNumbers() {
		$this->lineNumbers = true;
	}

	public function disableLineNumbers() {
		$this->lineNumbers = false;
	}

	public function startLineNumbersAt( $startLine ) {
		$this->startLine = (int)floor( $startLine );
	}

	public function setTabWidth( $tabWidth ) {
		$this->tabWidth = (int)floor( $tabWidth );
	}

	public function parseCode( $text, $lang = '' ) {
		$this->error = 0;
		$this->errorMessage = '';

		if ( $lang === '' ) {
			$lang = $this->defaultLang;
		}

		$cssPath = $this->makeTempPath( '.css' );
		if ( $cssPath === null ) {
			$this->error = -2;
			$this->errorMessage = 'Failed to create temporary file.';
			return null;
		}

		$args = [
			$this->binary,
			'--fragment',
			'--syntax=' . $lang,
			'--style-outfile=' . $cssPath,
		];

		if ( $this->lineNumbers ) {
			$args[] = '--linenumbers';
			if ( $this->startLine !== null ) {
				$args[] = '--line-number-start=' . $this->startLine;
			}
		}

		if ( $this->tabWidth > 0 ) {
			$args[] = '--replace-tabs=' . $this->tabWidth;
		}

		$command = Shell::command( ...$args )
			->input( $text );

		if ( method_exists( $command, 'timeout' ) ) {
			$command->timeout( $this->timeout );
		} elseif ( method_exists( $command, 'setTimeout' ) ) {
			$command->setTimeout( $this->timeout );
		}

		$result = $command->execute();

		if ( !$result->isOK() ) {
			$this->error = (int)$result->getExitCode();
			$this->errorMessage = trim( $result->getStderr() );
			return null;
		}

		$out = $result->getStdout();

		if ( is_file( $cssPath ) ) {
			$this->stylesheet = file_get_contents( $cssPath );
			@unlink( $cssPath );
		}

		return $out;
	}

	public function getStylesheet() {
		return $this->stylesheet;
	}

	public function getLanguageList() {
		$langs = [];
		if ( !is_dir( $this->langRoot ) ) {
			return $langs;
		}

		$dir = opendir( $this->langRoot );
		if ( !$dir ) {
			return $langs;
		}

		while ( ( $item = readdir( $dir ) ) !== false ) {
			if ( preg_match( '/^(.*)\\.lang$/', $item, $matches ) ) {
				$langs[] = $matches[1];
			}
		}

		closedir( $dir );
		sort( $langs );
		return $langs;
	}

	private function makeTempPath( $suffix ) {
		$dir = $this->tempDir ?: wfTempDir();
		if ( !$dir || !is_dir( $dir ) ) {
			return null;
		}

		$base = tempnam( $dir, 'ashighlight-' );
		if ( $base === false ) {
			return null;
		}

		@unlink( $base );
		return $base . $suffix;
	}
}
