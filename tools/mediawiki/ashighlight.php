<?php
/**
 * Syntax-highlight extension for MediaWiki 1.9 using Andre Simon's 'highlight'
 * Copyright (C) 2007 John Pye <john@curioussymbols.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * http://www.gnu.org/copyleft/gpl.html
 */

/**
 * @addtogroup Extensions
 * @author John Pye
 *
 * This extension wraps the Andre Simon highlighter: http://www.andre-simon.de/
 *
 * Modelled closely on the sourcecode for the GeSHi highlighter for
 * mediawiki, by Brion Vibber http://www.mediawiki.org/
 *
 * A language is specified like: <source lang="c">void main() {}</source>
 * If you forget, or give an unsupported value, the extension spits out
 * some help text and a list of all supported languages.
 */

if( !defined( 'MEDIAWIKI' ) )
	die();

$wgExtensionFunctions[] = 'ashighlightSetup';
$wgExtensionCredits['parserhook']['ashighlight'] = array(
	'name'          => 'ashighlight',
	'author'        => 'John Pye',
	'description'   => "Provides syntax highlighting using [http://www.andre-simon.de/ Andre Simon's 'highlight']",
);
$wgHooks['LoadAllMessages'][] = 'ashighlightLoadMessages';

$ASH_CSS=array();

function ashighlightSetup() {
	global $wgParser;
	$wgParser->setHook( 'source', 'ashighlightHook' );
	$wgHooks['SkinTemplateSetupPageCss'][]='ashighlightCss';
}

function ashighlightLoadMessages() {
	static $loaded = false;
	if ( $loaded ) {
		return;
	}
	global $wgMessageCache;
	require_once( dirname( __FILE__ ) . '/ashighlight.i18n.php' );
	foreach( efashighlightMessages() as $lang => $messages )
		$wgMessageCache->addMessages( $messages, $lang );
}

// called anytime a 'source' tag is seen in the wikitext...
function ashighlightHook( $text, $params = array(), $parser ) {
	if ( !class_exists( 'ASHighlight' ) ) {
		require( 'ashighlight.class.php' );
	}
	ashighlightLoadMessages();
	return isset( $params['lang'] )
		? ashighlightFormat( trim( $text ), $params, $parser )
		: ashighlightHelp();
}

// format the passed $text in the language $params['lang']
function ashighlightFormat( $text, $params, $parser ) {
	$lang = $params['lang'];
	if ( !preg_match( '/^[A-Za-z_0-9-]*$/', $lang ) ) {
		return ashighlightHelp( wfMsgHtml( 'ashighlight-err-language' ) );
	}

	$ash = new ASHighlight();

	$ash->set_encoding('UTF-8');

	if(isset($params['tabwidth'])){
		$ash->set_tab_width($params['tabwidth']);
	}

	if(isset( $params['line'] ) ) {
		$ash->enable_line_numbers();
	}

	if(isset( $params['start'] ) ) {
		$ash->start_line_numbers_at( $params['start'] );
	}

	$out = $ash->parse_code($text,$lang);

	if($ash->error){
		return ashighlightHelp($ash->errmsg);
	}else{
		// Per-language class for stylesheet
		//$ASH_CSS[]=
		$css = 
			"<style type=\"text/css\">/*<![CDATA[*/\n".$ash->get_stylesheet()."/*]]>*/</style>\n";
		$out = $css . "<pre>$out</pre>";
		return $out;
	}
}

/**
	Return a syntax help message
	@param string $error HTML error message
*/
function ashighlightHelp( $error = false ) {
	return ashighlightError(
		( $error ? "<p>$error</p>" : '' ) .
		'<p>' . wfMsg( 'ashighlight-specify' ) . ' ' .
		'<samp>&lt;source lang=&quot;html&quot;&gt;...&lt;/source&gt;</samp></p>' .
		'<p>' . wfMsg( 'ashighlight-supported' ) . '</p>' .
		ashighlightFormatList( ashighlightLanguageList() ) );
}

/**
 * Put a red-bordered div around an HTML message
 * @param string $contents HTML error message
 * @return HTML
 */
function ashighlightError( $contents ) {
	return "<div style=\"border:solid red 1px; padding:.5em;\">$contents</div>";
}

function ashighlightFormatList( $list ) {
	return empty( $list )
		? wfMsg( 'ashighlight-err-loading' )
		: '<p style="padding:0em 1em;">' .
			implode( ', ', array_map( 'ashighlightListItem', $list ) ) .
			'</p>';
}

function ashighlightListItem( $item ) {
	return "<samp>" . htmlspecialchars( $item ) . "</samp>";
}

function ashighlightLanguageList() {
	$langs = array();
	$langroot = @opendir( ASHIGHLIGHT_LANG_ROOT );
	if( $langroot ) {
		while( $item = readdir( $langroot ) ) {
			if( preg_match( '/^(.*)\\.lang$/', $item, $matches ) ) {
				$langs[] = $matches[1];
			}
		}
		closedir( $langroot );
	}
	sort( $langs );
	return $langs;
}

function ashighlightCss(&$out){
	//foreach($ASH_CSS as $css){
	//	$out.=$css."\n\n";
	//}
	$out.="kwa{font:bold}";
	return true;
}

?>
