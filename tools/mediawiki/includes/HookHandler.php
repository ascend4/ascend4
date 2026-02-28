<?php

namespace ASHighlight;

use ASHighlight;
use Html;
use MediaWiki\Logger\LoggerFactory;
use MediaWiki\MediaWikiServices;
use Parser;

class HookHandler {
	public static function onParserFirstCallInit( Parser $parser ) {
		$parser->setHook( 'source', [ self::class, 'onSourceTag' ] );
		return true;
	}

	public static function onSourceTag( $text, array $params, Parser $parser ) {
		$parser->getOutput()->addModuleStyles( [ 'ext.ashighlight.styles' ] );

		if ( !isset( $params['lang'] ) || $params['lang'] === '' ) {
			return self::helpMessage();
		}

		$lang = $params['lang'];
		if ( !preg_match( '/^[A-Za-z_0-9-]*$/', $lang ) ) {
			return self::helpMessage(
				Html::element( 'span', [], wfMessage( 'ashighlight-err-language' )->text() )
			);
		}

		$highlighter = self::newHighlighter();
		$highlighter->setEncoding( 'UTF-8' );

		if ( isset( $params['tabwidth'] ) ) {
			$highlighter->setTabWidth( $params['tabwidth'] );
		}

		if ( isset( $params['line'] ) ) {
			$highlighter->enableLineNumbers();
		}

		if ( isset( $params['start'] ) ) {
			$highlighter->startLineNumbersAt( $params['start'] );
		}

		$logger = LoggerFactory::getInstance( 'ASHighlight' );
		$langs = $highlighter->getLanguageList();
		$logger->info( 'ASHighlight render requested.', [
			'lang' => $lang,
			'langRoot' => MediaWikiServices::getInstance()->getMainConfig()->get( 'ASHighlightLangRoot' ),
			'langCount' => count( $langs ),
			'langFound' => in_array( $lang, $langs, true ),
		] );
		if ( $langs === [] ) {
			$logger->warning( 'Language list empty; langRoot may be wrong.', [
				'langRoot' => MediaWikiServices::getInstance()->getMainConfig()->get( 'ASHighlightLangRoot' ),
			] );
		} elseif ( !in_array( $lang, $langs, true ) ) {
			$logger->warning( 'Requested language not found in langDefs.', [
				'lang' => $lang,
				'langRoot' => MediaWikiServices::getInstance()->getMainConfig()->get( 'ASHighlightLangRoot' ),
			] );
			return self::helpMessage(
				Html::element( 'span', [], wfMessage( 'ashighlight-err-language' )->text() )
			);
		}

		$out = $highlighter->parseCode( $text, $lang );
		if ( $highlighter->error ) {
			$logger->error( 'Highlight failed.', [
				'lang' => $lang,
				'error' => $highlighter->error,
				'message' => $highlighter->errorMessage,
			] );
			$error = $highlighter->errorMessage ?: wfMessage( 'ashighlight-err-language' )->text();
			return self::helpMessage( Html::element( 'span', [], $error ) );
		}

		return Html::rawElement( 'pre', [ 'class' => 'ashighlight hl' ], $out );
	}

	private static function helpMessage( $error = null ) {
		$errorHtml = $error ? Html::rawElement( 'p', [], $error ) : '';
		$specify = wfMessage( 'ashighlight-specify' )->escaped();
		$supported = wfMessage( 'ashighlight-supported' )->escaped();
		$example = Html::element( 'samp', [], '<source lang="html">...</source>' );

		$body =
			$errorHtml .
			Html::rawElement( 'p', [], $specify . ' ' . $example ) .
			Html::rawElement( 'p', [], $supported ) .
			self::formatLanguageList( self::newHighlighter()->getLanguageList() );

		return self::formatError( $body );
	}

	private static function formatError( $contents ) {
		return Html::rawElement(
			'div',
			[ 'style' => 'border:solid red 1px; padding:.5em;' ],
			$contents
		);
	}

	private static function formatLanguageList( array $list ) {
		if ( $list === [] ) {
			return wfMessage( 'ashighlight-err-loading' )->escaped();
		}

		$items = array_map(
			static function ( $item ) {
				return Html::element( 'samp', [], $item );
			},
			$list
		);

		return Html::rawElement( 'p', [ 'style' => 'padding:0em 1em;' ], implode( ', ', $items ) );
	}

	private static function newHighlighter() {
		$config = MediaWikiServices::getInstance()->getMainConfig();
		return new ASHighlight( [
			'binary' => $config->get( 'ASHighlightBinary' ),
			'langRoot' => $config->get( 'ASHighlightLangRoot' ),
			'timeout' => $config->get( 'ASHighlightTimeout' ),
			'tempDir' => $config->get( 'ASHighlightTempDir' ),
		] );
	}
}
